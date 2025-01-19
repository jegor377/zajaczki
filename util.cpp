#include "util.hpp"

void init_packet_type() {
    int blocklenths[NITEMS] = {1, 1, 1};
    MPI_Datatype types[NITEMS] = {MPI_INT, MPI_INT, MPI_INT};

    MPI_Aint offsets[NITEMS];
    offsets[0] = offsetof(packet_t, ts);
    offsets[1] = offsetof(packet_t, src);
    offsets[2] = offsetof(packet_t, value);

    MPI_Type_create_struct(NITEMS, blocklenths, offsets, types, &MPI_PACKET_T);
}

std::string tag2string(enum PacketType tag) {
    switch (tag)
    {
    case TYPE:
        return "TYPE";
    case ACK:
        return "ACK";
    case WANT:
        return "WANT";
    case COME:
        return "COME";
    case START:
        return "START";
    case OCCUPIED:
        return "OCCUPIED";
    case FREE:
        return "FREE";
    default:
        return "UNKNOWN";
    }
}