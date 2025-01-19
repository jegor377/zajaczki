#pragma once

#include "mpi_header.hpp"

#include <string>

#define NITEMS 3

MPI_Datatype MPI_PACKET_T;

typedef struct {
    int ts;
    int src;
    int value;

    bool operator<(const packet_t& other) const {
        return ts < other.ts;
    }
} packet_t;

enum State
{
    THINK,
    INIT,
    IDLE,
    WANT,
    DECIDE,
    WAIT,
    PARTY,
    FINISH // nigdy się nie wydarzy
};

enum ProcessType
{
    UNDEFINED = -1,
    HARE,
    BEAR
};

enum PacketType
{
    TYPE = 0,
    ACK,
    WANT,
    COME,
    START,
    OCCUPIED,
    FREE
};

void init_packet_type();
std::string tag2string(enum PacketType tag);