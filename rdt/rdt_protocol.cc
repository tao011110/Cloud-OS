#include "rdt_protocol.h"

int window_size = 10;
int header_size = 12;
int max_payload_size = RDT_PKTSIZE - header_size;

// TODO: The Internet checksum, to be redo later!
short CheckSum(struct packet* pkt){
    unsigned long checksum = 0;
    for(int i = 2; i < RDT_PKTSIZE; i += 2){
        checksum += *(short *)(&(pkt->data[i]));
    }
    while(checksum >> 16){
        checksum = (checksum >> 16) + (checksum & 0xffff);
    }

    return ~checksum;
}