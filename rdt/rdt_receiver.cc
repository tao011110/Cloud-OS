/*
 * FILE: rdt_receiver.cc
 * DESCRIPTION: Reliable data transfer receiver.
 * NOTE: This implementation assumes there is no packet loss, corruption, or 
 *       reordering.  You will need to enhance it to deal with all these 
 *       situations.  In this implementation, the packet format is laid out as 
 *       the following:
 *       
 *       |<-  2 byte  ->|<-  4 byte  ->|<-   the rest  ->|
 *       |   checksum   |    ack seq   |<-   nothing   ->|
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rdt_struct.h"
#include "rdt_receiver.h"
#include "rdt_protocol.h"

message *msg;
packet *rev_window;
bool acks[10];

// the cursor always points to the first unreceived byte in the message
int rev_msg_cursor = 0;
// the sequence number of ack
int ack_seq = 0;

/* receiver initialization, called once at the very beginning */
void Receiver_Init()
{
    fprintf(stdout, "At %.2fs: receiver initializing ...\n", GetSimulationTime());

    for(int i = 0; i < window_size; i++){
        acks[i] = 0;
    }
    rev_window = (packet *)malloc(window_size * sizeof(packet));
    memset(rev_window, '\0', window_size * sizeof(packet));

    msg = (struct message*) malloc(sizeof(struct message));
    msg->size = 0;
}

/* receiver finalization, called once at the very end.
   you may find that you don't need it, in which case you can leave it blank.
   in certain cases, you might want to use this opportunity to release some 
   memory you allocated in Receiver_init(). */
void Receiver_Final()
{
    fprintf(stdout, "At %.2fs: receiver finalizing ...\n", GetSimulationTime());
    free(rev_window);
    delete[] msg->data;
    free(msg);
}

void Send_Ack(int ack) {
    packet pkt;
    memset(&pkt.data, '\0', RDT_PKTSIZE);
    memcpy(pkt.data + sizeof(short), &ack, sizeof(int));
    short checksum = CheckSum(&pkt);
    memcpy(pkt.data, &checksum, sizeof(short));

    printf("send ack %d\n", ack);
    Receiver_ToLowerLayer(&pkt);
}

/* event handler, called when a packet is passed from the lower layer at the 
   receiver */
void Receiver_FromLowerLayer(struct packet *pkt)
{
    /* construct a message and deliver to the upper layer */
    short checksum;
    memcpy(&checksum, pkt->data, sizeof(short));
    if(checksum != CheckSum(pkt)){ 
        printf("%d checksum is not OK! %d\n", checksum, CheckSum(pkt));
        return;
    }
    int pkt_seq = 0;
    int payload_size = 0;
    memcpy(&pkt_seq, pkt->data + sizeof(short), sizeof(int));

    if(pkt_seq > ack_seq && pkt_seq < ack_seq + window_size){
        printf("wocccc221\n");
        if(!acks[pkt_seq % window_size]){
            memcpy(&(rev_window[pkt_seq % window_size].data), pkt->data, RDT_PKTSIZE);
            acks[pkt_seq % window_size] = true;
        }
        Send_Ack(ack_seq - 1);
        return;
    }
    else{
        if(pkt_seq != ack_seq){
            printf("pkt_seq is %d while ack_seq is %d\n", pkt_seq, ack_seq);
            Send_Ack(ack_seq - 1);
            return;
        }
    }

    while(true){
        bool is_spilt = false;
        ack_seq++;
        memcpy(&payload_size, pkt->data + header_size - sizeof(char), sizeof(char));
        memcpy(&is_spilt, pkt->data + header_size - sizeof(bool) - sizeof(char), sizeof(bool));
        
        if(!rev_msg_cursor){
            if(msg->size){
                msg->size = 0;
                delete[] msg->data;
            }
            if(is_spilt){
                // the first pkt of a spilt msg
                memcpy(&(msg->size), pkt->data + header_size, sizeof(int));
                msg->data = new char[msg->size];
                memcpy(msg->data, pkt->data + header_size + sizeof(int), payload_size - sizeof(int));
                rev_msg_cursor += payload_size - sizeof(int);
            }
            else{
                msg->size = payload_size;
                msg->data = new char[msg->size];
                memcpy(msg->data, pkt->data + header_size, payload_size);
                rev_msg_cursor += payload_size;
            }
        }
        else{
            printf("%d and %d\n", rev_msg_cursor, msg->size);
            memcpy(msg->data + rev_msg_cursor, pkt->data + header_size, payload_size);
            rev_msg_cursor += payload_size;
        }
        if(rev_msg_cursor == msg->size){
            printf("%d wocccc %s\n", rev_msg_cursor, msg->data);
            rev_msg_cursor = 0;
            Receiver_ToUpperLayer(msg);
        }
        else{
            printf("%d while  %d\n", rev_msg_cursor, msg->size);
        }

        if(acks[ack_seq % window_size]){
            pkt = &rev_window[ack_seq % window_size];
            memcpy(&pkt_seq, pkt->data + sizeof(short), sizeof(int));
            acks[ack_seq % window_size] = false;
        }
        else{
            printf("we break\n");
            break;
        }
    }

    Send_Ack(pkt_seq);
}
