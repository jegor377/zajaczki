#pragma once

typedef struct {
    int ts;
    int src;

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
    PARTY
};

enum ProcessType
{
    UNDEFINED,
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