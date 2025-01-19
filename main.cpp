#include "mpi_header.hpp"
#include <ctime>
#include <unistd.h>
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <queue>
#include <cmath>
#include <random>

#define DEBUG
#include "debug.hpp"
#include "util.hpp"

using namespace std;

FILE *log_file;
enum State state = INIT;
vector<enum ProcessType> types;
vector<bool> meadows;
priority_queue<pair<int, packet_t>, vector<pair<int, packet_t>>, std::greater<>> commQueue;
priority_queue<pair<int, int>> waitQueue;
bool isLeader = false;
enum ProcessType proc_type;
int processes_count, proc_rank;
int ts = 0;

std::random_device rd;

void get_timestamp(char *timestamp, size_t size)
{
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);

    // Format: YYYY-MM-DDTHH:MM:SS
    strftime(timestamp, size, "%Y-%m-%dT%H:%M:%S", tm_info);
}

void log_state_change(const char *state, bool with_header)
{
    char timestamp[32];
    get_timestamp(timestamp, sizeof(timestamp));

    // If this is the first entry, write a header row
    if (with_header)
    {
        fprintf(log_file, "timestamp,process_id,state\n");
    }

    // Write the timestamp, rank, and state in CSV format
    fprintf(log_file, "%s,%d,%s\n", timestamp, proc_rank, state);
    fflush(log_file); // ensure data is written to disk

    sleep(1); // make everything visible
}



void send_packet(int value, int destination, int tag)
{
    packet_t pkt = {
        .ts = ++ts,
        .src = proc_rank,
        .value = value
    };
    if(destination != -1) {
        MPI_Send(&pkt, 1, MPI_PACKET_T, destination, tag, MPI_COMM_WORLD);
        debug("Wysyłam %s do %d\n", tag2string((enum PacketType)tag), destination);
    } else {
        MPI_Bcast(&pkt, 1, MPI_PACKET_T, proc_rank, MPI_COMM_WORLD);
        debug("Wysyłam %s do wszystkich\n", tag2string((enum PacketType)tag));
    }
}

void get_packet(packet_t *packet, MPI_Status *status) {
    MPI_Recv(packet, 1, MPI_PACKET_T, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, status);
    ts = max(packet->ts, ts) + 1;
}

void finalize() {
    MPI_Type_free(&MPI_PACKET_T);
    MPI_Finalize();
}

int main(int argc, char **argv)
{
    std::mt19937 gen(rd());

	MPI_Init(&argc,&argv);

	MPI_Comm_size(MPI_COMM_WORLD, &processes_count);
	MPI_Comm_rank(MPI_COMM_WORLD, &proc_rank);

    types.resize(processes_count, UNDEFINED);

    char filename[64];
    snprintf(filename, sizeof(filename), "out/process_%d.csv", proc_rank);

	log_file = fopen(filename, "w");
    if (!log_file)
    {
        fprintf(stderr, "Error: unable to open log log_file for rank %d\n", proc_rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

	log_state_change("INIT", true);

    init_packet_type();

    MPI_Status status;
    packet_t packet;

    while(state != FINISH) {
        switch(state) {
            case INIT: {
                std::uniform_int_distribution<> distrib(0, 1);
                proc_type = (enum ProcessType)((int)distrib(gen));
                debug("Proces przyjal typ: %d\n", proc_type);
                send_packet(proc_type, -1, TYPE);
                int proc_to_discover = processes_count;
                vector<bool> discovered(processes_count);
                while(proc_to_discover > 0) {
                    get_packet(&packet, &status);
                    switch ((enum PacketType)status.MPI_TAG)
                    {
                        case TYPE: {
                            if(!discovered[packet.src]) {
                                discovered[packet.src] = true;
                                proc_to_discover--;
                            }
                            types[packet.src] = (enum ProcessType)packet.value;
                        } break;
                        case PacketType::WANT: {
                            send_packet(0, packet.src, ACK);
                        } break;
                        default:
                            break;
                    }
                }
                state = THINK;
            } break;
            case THINK: {
                std::uniform_int_distribution<> distrib(0, 1);
                if(distrib(gen) == 0) {
                    state = IDLE;
                } else {
                    state = State::WANT;
                }
            } break;
            case IDLE: {
                ;
            } break;
        }
    }

    finalize();

    return 0;
}