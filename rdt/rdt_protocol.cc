#include "rdt_protocol.h"

int window_size = 10;
int header_size = 8;
int max_payload_size = 121;

short CheckSum(struct packet* pkt){
    unsigned long checksum = 0;
    // skip the checksum field
    for(int i = 2; i < RDT_PKTSIZE; i += 2){
        checksum += *(short *)(&(pkt->data[i]));
    }
    while(checksum >> 16){
        checksum = (checksum >> 16) + (checksum & 0xffff);
    }

    return ~checksum;
}