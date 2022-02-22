#ifndef _RDT_PROTOCOL_H_
#define _RDT_PROTOCOL_H_

#include "rdt_struct.h"

extern int window_size;
extern int header_size;
extern int max_payload_size;

short CheckSum(struct packet* pkt);

#endif