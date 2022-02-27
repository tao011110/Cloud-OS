/*
 * FILE: rdt_sender.cc
 * DESCRIPTION: Reliable data transfer sender.
 * NOTE: This implementation assumes there is no packet loss, corruption, or 
 *       reordering.  You will need to enhance it to deal with all these 
 *       situations.  In this implementation, the packet format is laid out as 
 *       the following:
 *       
 *       |<-  2 byte  ->|<-  4 byte  ->|<-  1 byte  ->|<-  1 byte  ->|<-  the rest ->|
 *       |   checksum   |  packet seq  | is spilt msg | payload size |<-  payload  ->|
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rdt_struct.h"
#include "rdt_sender.h"
#include "rdt_protocol.h"

double timeout = 0.3;
int buffer_size = 10240;
message *buffer;
packet *window;
bool sack[102400];

// the sequence number of msg
int msg_seq = 0;
// the number of msgs to be sent
int msg_num = 0;
// the sequence number of msg to be sent
int msg_tosend_seq = 0;
// the cursor always points to the first unsent byte in the message
int msg_cursor = 0;

// the sequence number of pkt
int pkt_seq = 0;
// the number of pkts to be sent
int pkt_num = 0;
// the sequence number of pkt to be sent
int pkt_tosend = 0;
// the sequence number of next pkt to be put in the window
int next_pkt = 0;
// the sequence number of the expected ack
int expect_ack = 0;

/* sender initialization, called once at the very beginning */
void Sender_Init()
{
    buffer = (message *)malloc(buffer_size * sizeof(message));
    memset(buffer, 0, buffer_size * sizeof(message));
    window = (packet *)malloc(window_size * sizeof(packet));
    memset(window, 0, window_size * sizeof(packet));
    for(int i = 0; i < 102400; i++){
        sack[i] = false;
    }

    fprintf(stdout, "At %.2fs: sender initializing ...\n", GetSimulationTime());
}

/* sender finalization, called once at the very end.
   you may find that you don't need it, in which case you can leave it blank.
   in certain cases, you might want to take this opportunity to release some 
   memory you allocated in Sender_init(). */
void Sender_Final()
{
    for(int i = 0; i < buffer_size; i++){
        if(buffer[i].size){
            free(buffer[i].data);
        }
    }
    free(buffer);
    free(window);

    fprintf(stdout, "At %.2fs: sender finalizing ...\n", GetSimulationTime());
}

void Send_Pkt(){
    packet pkt;
    while(pkt_tosend < next_pkt){
        if(!sack[pkt_tosend]){
            memcpy(&pkt, &(window[pkt_tosend % window_size]), sizeof(packet));
            /* send it out through the lower layer */
            Sender_ToLowerLayer(&pkt);
            // printf("has sent out pkg %d\n", pkt_tosend);
        }
        pkt_tosend++;
    }
}

void Fill_Window(){
    // printf("%d send msg size is %d\n", msg_tosend_seq, msg.size);
    message msg = buffer[msg_tosend_seq];
    int payload_size = 0;

    /* reuse the same packet data structure */
    packet pkt;

    while(pkt_num < window_size && msg_tosend_seq < msg_seq){
        /* split the message if it is too big */
        if(msg.size - msg_cursor > max_payload_size) {
            payload_size = max_payload_size;
            bool is_spilt = true;

            /* fill in the packet */
            memcpy(pkt.data + sizeof(short), &pkt_seq, sizeof(int));
            memcpy(pkt.data + header_size - sizeof(char) - sizeof(bool), &is_spilt, sizeof(bool));
            memcpy(pkt.data + header_size - sizeof(char), &payload_size, sizeof(char));

            // we put the msg size in the first spilt pkt
            if(!msg_cursor){
                payload_size -= sizeof(int);
                memcpy(pkt.data + header_size, &(msg.size), sizeof(int));
                memcpy(pkt.data + header_size + sizeof(int), msg.data + msg_cursor, max_payload_size - sizeof(int));
            }
            else{
                memcpy(pkt.data + header_size, msg.data + msg_cursor, max_payload_size);
            }

            // calc the checksum of the pkg
            short checksum = CheckSum(&pkt);
            memcpy(pkt.data, &checksum, sizeof(short));
            // printf("checksum %d  and  seq  %d\n", checksum, pkt_seq);
            
            // put the packet into the window
            memcpy(&(window[pkt_seq % window_size]), &pkt, sizeof(packet));

            /* move the cursor */
            msg_cursor += payload_size;
        }
        else{
            payload_size = msg.size - msg_cursor;
            bool is_spilt = true;

            /* fill in the packet */
            memcpy(pkt.data + sizeof(short), &pkt_seq, sizeof(int));
            if(!msg_cursor){
                is_spilt = false;
            }
            
            memcpy(pkt.data + header_size - sizeof(char) - sizeof(bool), &is_spilt, sizeof(bool));
            memcpy(pkt.data + header_size - sizeof(char), &payload_size, sizeof(char));
            memcpy(pkt.data + header_size, msg.data + msg_cursor, payload_size);
            // calc the checksum of the pkg
            short checksum = CheckSum(&pkt);
            memcpy(pkt.data, &checksum, sizeof(short));
            // printf("checksum %d  and  seq  %d\n", checksum, pkt_seq);
            
            // put the packet into the window
            memcpy(&(window[pkt_seq % window_size]), &pkt, sizeof(packet));

            /* try to handle the next message */
            msg_num--;
            msg_tosend_seq++;
            if(msg_tosend_seq < msg_seq){
                msg = buffer[msg_tosend_seq];
            }
            msg_cursor = 0;
            // printf("msg_tosend_seq is %d and msg_seq is %d\n", msg_tosend_seq, msg_seq);
        }
        // printf("we sent pkt %d has the payload of %d\n", pkt_seq, payload_size);
        pkt_seq++;
        pkt_num++;
    }
    next_pkt = pkt_seq;

    Send_Pkt();
}

/* event handler, called when a message is passed from the upper layer at the 
   sender */
void Sender_FromUpperLayer(struct message *msg){
    if(msg_num > buffer_size){
        printf("the buffer is too small now!\n");
    }
    int index = msg_seq % buffer_size;
    if(buffer[index].size){
        free(buffer[index].data);
    }
    buffer[index].size = msg->size;
    buffer[index].data = (char *)malloc(msg->size);
    memcpy(buffer[index].data, msg->data, msg->size);
    msg_seq++;
    msg_num++;

    // the timer is working, msg stays in the buffer
    if(Sender_isTimerSet()){
        return;
    }
    Sender_StartTimer(timeout);
    // put the packets into the window
    Fill_Window();
}

/* event handler, called when a packet is passed from the lower layer at the 
   sender */
void Sender_FromLowerLayer(struct packet *pkt){
    short checksum;
    memcpy(&checksum, pkt->data, sizeof(short));
    if(checksum != CheckSum(pkt)){
        return;
    }
    // printf("%d send checksum ! %d\n", checksum, CheckSum(pkt));

    int ack_num = 0;
    memcpy(&ack_num, pkt->data + sizeof(short), sizeof(int));
    // printf("%d get ack_num %d\n", checksum, ack_num);
    if(ack_num < 0 || ack_num >= pkt_seq){
        return;
    }
    sack[ack_num] = true;

    // all the pkts with sequence number no more than ack are recieved successfully
    if(expect_ack <= ack_num){
        Sender_StartTimer(timeout);
        if(expect_ack == ack_num){
            pkt_num--;
            expect_ack++;
            for(int i = expect_ack; i < pkt_seq; i++){
                if(!sack[i]){
                    break;
                }
                pkt_num--;
                expect_ack++;
            }
            Fill_Window();
        }
    }

    // all the pkts in the window get acks
    bool flag = true;
    for(int i = 1; i <= 10; i++){
        if(!sack[pkt_seq - i]){
            flag = false;
            break;
        }
    }
    if(flag){
        Sender_StopTimer();
    }
}

/* event handler, called when the timer expires */
void Sender_Timeout(){
    pkt_tosend = expect_ack;
    Sender_StartTimer(timeout);
    Fill_Window();
}
