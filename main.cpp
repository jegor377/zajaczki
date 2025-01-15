#include <mpi.h>
#include <ctime>
#include <unistd.h>
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <queue>

#include "util.hpp"

using namespace std;

FILE *log_file;
enum State state = INIT;
vector<enum ProcessType> types;
vector<bool> meadows;
priority_queue<pair<int, packet_t>, vector<pair<int, packet_t>>, std::greater<>> commQueue;
priority_queue<pair<int, int>> waitQueue;
bool isLeader = false;
enum ProcessType processType;
int processesCount, procRank;

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
    fprintf(log_file, "%s,%d,%s\n", timestamp, procRank, state);
    fflush(log_file); // ensure data is written to disk

    sleep(1); // make everything visible
}

int main(int argc, char **argv)
{
	MPI_Init(&argc,&argv);

	MPI_Comm_size(MPI_COMM_WORLD, &processesCount);
	MPI_Comm_rank(MPI_COMM_WORLD, &procRank);

	char filename[64];
    snprintf(filename, sizeof(filename), "out/process_%d.csv", procRank);

	log_file = fopen(filename, "w");
    if (!log_file)
    {
        fprintf(stderr, "Error: unable to open log log_file for rank %d\n", procRank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

	log_state_change("INIT", true);

	cout << "Hello world: " << procRank << " of " << processesCount << "\n";
	MPI_Finalize();
}