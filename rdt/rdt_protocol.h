#ifndef _RDT_PROTOCOL_H_
#define _RDT_PROTOCOL_H_

#include "rdt_struct.h"

// there will be something wrong when compiling without extern here!
extern int window_size;
extern int header_size;
extern int max_payload_size;

short CheckSum(struct packet* pkt);

#endif