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
#include <pthread.h>
#include <semaphore.h>

#define DEBUG
#include "debug.hpp"
#include "util.hpp"

using namespace std;

FILE *log_file;
enum State state = INIT;
vector<enum ProcessType> types;
vector<pair<bool, int>> meadows;
priority_queue<pair<int, pair<enum PacketType, packet_t>>, vector<pair<int, pair<enum PacketType, packet_t>>>, std::greater<>> commQueue;
sem_t commQueueSem;
priority_queue<pair<int, int>, vector<pair<int, int>>, std::greater<>> waitQueue;
bool isLeader = false;
enum ProcessType proc_type;
int proc_value;
int processes_count, proc_rank;
int ts = 0;
pthread_t comm_thread;
pthread_mutex_t state_mut = PTHREAD_MUTEX_INITIALIZER, comm_mut = PTHREAD_MUTEX_INITIALIZER, ts_mut = PTHREAD_MUTEX_INITIALIZER;
int desired_meadow;

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



int send_packet(int value, int destination, int tag)
{
    pthread_mutex_lock(&ts_mut);
    int initial_ts = ++ts;
    packet_t pkt = {
        .ts = initial_ts,
        .src = proc_rank,
        .value = value
    };
    pthread_mutex_unlock(&ts_mut);
    if(destination != -1) {
        MPI_Send(&pkt, 1, MPI_PACKET_T, destination, tag, MPI_COMM_WORLD);
        debug("Wysyłam %s do %d\n", tag2string((enum PacketType)tag), destination);
    } else {
        MPI_Bcast(&pkt, 1, MPI_PACKET_T, proc_rank, MPI_COMM_WORLD);
        debug("Wysyłam %s do wszystkich\n", tag2string((enum PacketType)tag));
    }
    return initial_ts;
}

void get_packet(packet_t *packet, MPI_Status *status) {
    MPI_Recv(packet, 1, MPI_PACKET_T, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, status);
    pthread_mutex_lock(&ts_mut);
    ts = max(packet->ts, ts) + 1;
    pthread_mutex_unlock(&ts_mut);
}

void finalize() {
    pthread_mutex_destroy(&state_mut);
    pthread_mutex_destroy(&comm_mut);
    pthread_mutex_destroy(&ts_mut);
    pthread_join(comm_thread, NULL);
    sem_destroy(&commQueueSem);
    MPI_Type_free(&MPI_PACKET_T);
    MPI_Finalize();
}

void change_state(enum State new_state) {
    pthread_mutex_lock(&state_mut);
    if (state==FINISH) { 
	    pthread_mutex_unlock(&state_mut);
        return;
    }
    state = new_state;
    pthread_mutex_unlock(&state_mut);
}

void save_packet(packet_t packet, int type) {
    pthread_mutex_lock(&comm_mut);
    commQueue.push({packet.ts, {(enum PacketType)type, packet}});
    pthread_mutex_unlock(&comm_mut);
    sem_post(&commQueueSem);
}

void load_packet(packet_t *packet, enum PacketType *type) {
    sem_wait(&commQueueSem);
    pthread_mutex_lock(&comm_mut);
    *type = commQueue.top().second.first;
    *packet = commQueue.top().second.second;
    commQueue.pop();
    pthread_mutex_unlock(&comm_mut);
}

void load_packet_nb(packet_t *packet, enum PacketType *type) {
    int value;
    sem_getvalue(&commQueueSem, &value);
    if(value > 0) {
        load_packet(packet, type);
    }
}

void *comm_thread_handler(void *ptr) {
    MPI_Status status;
    packet_t packet;

    while(state != FINISH) {
        get_packet(&packet, &status);
        save_packet(packet, status.MPI_TAG);
    }
}

void handle_triggers(packet_t *packet, enum PacketType type) {
    switch(type) {
    case OCCUPIED:
        break;
    case FREE:
        break;
    case TYPE:
        break;
    default:
        break;
    }
}

int main(int argc, char **argv)
{
    std::mt19937 gen(rd());

	MPI_Init(&argc,&argv);

	MPI_Comm_size(MPI_COMM_WORLD, &processes_count);
	MPI_Comm_rank(MPI_COMM_WORLD, &proc_rank);

    types.resize(processes_count, UNDEFINED);

    meadows.resize(10);
    for (int i = 0; i < 10; i++) {
        meadows[i] = {false, 5};
    }

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

    sem_init(&commQueueSem, 0, 0);
    pthread_create(&comm_thread, NULL, comm_thread_handler, 0);

    packet_t packet;
    enum PacketType type;

    while(state != FINISH) {
        switch(state) {
            case INIT: {
                std::uniform_int_distribution<> distrib(0, 1);
                proc_type = (enum ProcessType)((int)distrib(gen));
                if(proc_type == HARE) {
                    proc_value = 1;
                } else {
                    proc_value = 4;
                }
                debug("Proces przyjal typ: %d\n", proc_type);
                send_packet(proc_type, -1, TYPE);
                int proc_to_discover = processes_count;
                vector<bool> discovered(processes_count, false);
                while(proc_to_discover > 0) {
                    load_packet(&packet, &type);
                    switch(type) {
                    case TYPE:
                        if(!discovered[packet.src]) {
                            discovered[packet.src] = true;
                            proc_to_discover--;
                        }
                        types[packet.src] = (enum ProcessType)packet.value;
                        break;
                    case WANT:
                        send_packet(0, packet.src, ACK);
                        break;
                    default:
                        handle_triggers(&packet, type);
                        break;
                    }
                }
                change_state(THINK);
            } break;
            case THINK: {
                std::uniform_int_distribution<> distrib(0, 1);
                if(distrib(gen) == 0) {
                    change_state(IDLE);
                } else {
                    change_state(State::WANT);
                }
            } break;
            case IDLE: {
                std::uniform_int_distribution<> distrib(1, 10);

                int secs = distrib(gen);
                for (int i = 0; i < secs; i++) {
                    sleep(1);
                    load_packet_nb(&packet, &type);
                    if(type == WANT) {
                        send_packet(0, packet.src, ACK);
                    } else {
                        handle_triggers(&packet, type);
                    }
                }

                change_state(THINK);
            } break;
            case State::WANT: {
                std::uniform_int_distribution<> distrib(0, meadows.size() - 1);
                desired_meadow = distrib(gen);
                while(meadows[desired_meadow].first && meadows[desired_meadow].second <= proc_value) {
                    desired_meadow = distrib(gen);
                }
                int send_ts = send_packet(desired_meadow, -1, WANT);
                int to_receive = processes_count;
                vector<bool> received(processes_count, false);
                bool got_come = false;
                while(to_receive > 0) {
                    load_packet(&packet, &type);
                    switch(type) {
                    case ACK:
                        if(!received[packet.src]) {
                            received[packet.src] = true;
                            to_receive--;
                        }
                        break;
                    case WANT:
                        if(packet.value != desired_meadow) {
                            send_packet(0, packet.src, ACK);
                        } else if(proc_type == HARE) {
                            bool is_my_priority_higher = (packet.ts > send_ts) || (packet.ts == send_ts && packet.src > proc_rank);
                            if(is_my_priority_higher) {
                                waitQueue.push({packet.ts, packet.src});
                            } else {
                                send_packet(0, packet.src, ACK);
                            }
                        }
                        break;
                    case COME:
                        change_state(WAIT);
                        got_come = true;
                        to_receive = 0;
                        break;
                    default:
                        handle_triggers(&packet, type);
                        break;
                    }
                }
                
                if(!got_come) {
                    change_state(DECIDE);
                }
            } break;
            case DECIDE: {
                send_packet(desired_meadow, -1, OCCUPIED);
                isLeader = true;
                // TODO: Dokończyć
            } break;
            case WAIT: {
                load_packet(&packet, &type);
                switch (type)
                {
                case START:
                    change_state(PARTY);
                    break;
                case WAIT:
                    waitQueue.push({packet.ts, packet.src});
                    break;
                default:
                    handle_triggers(&packet, type);
                    break;
                }
            } break;
            case PARTY: {
                // TODO: Dokończyć
            } break;
        }
    }

    finalize();

    return 0;
}