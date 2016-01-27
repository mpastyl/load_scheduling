/*
 * Copyright (c) 2007, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

/**
 * \file
 *         Testing the broadcast layer in Rime
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

#include "contiki.h"
#include "net/rime/rime.h"
#include "random.h"
#include "sys/node-id.h"
#include "dev/button-sensor.h"

#include "dev/leds.h"

//#include "shared_variables.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*---------------------------------------------------------------------------*/
PROCESS(example_broadcast_process, "Broadcast example");
PROCESS(start_2pc_process, "start_2pc_process");
//AUTOSTART_PROCESSES(&start_2pc_process);
/*---------------------------------------------------------------------------*/
//static struct unicast_conn uc;
//static struct broadcast_conn broadcast;

#define TIMEOUT		(2*CLOCK_SECOND)

/*static int naive_counter=0;
static int votes[6]; 
static int saved_seq_numbers[6];
static int num_nodes=6;
static int state=0; //state=1 we are in phase 1 of some broadcast so reject new incoming broadcasts
//int node_seq_number=0;  
static int flag_bcast_over_or_aborted=0;


//
static int local_seq_number=0;
//
*/


static process_event_t event_bcast_over;
static process_event_t event_2pc_succ;

struct message_2pc_struct{
        int id;
        int seq;
        char * msg;
};

/*static void
print_ipv6_addr(const uip_ipaddr_t *ip_addr) {
    int i;
    for (i = 0; i < 16; i++) {
        printf("%02x", ip_addr->u8[i]);
    }
}
*/

static void
broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from)
{
  printf("broadcast message received from %d.%d: '%s'\n",
         from->u8[0], from->u8[1], (char *)packetbuf_dataptr());
	 
  naive_counter++;


  struct message_2pc_struct *received_message;
  received_message=packetbuf_dataptr();
  int sender_id=received_message->id;
  int sender_seq=received_message->seq;
  char * sender_msg=received_message->msg;
  printf("sender %d with seq %d says : %s\n",sender_id,sender_seq, sender_msg);

  if (sender_seq<saved_seq_numbers[sender_id-1]) {
	printf(" smaller seq number -> ignore\n");
        return;
  }
  //clock_delay(random_rand()*(CLOCK_SECOND/100));
  printf("replaying back with a unicast\n");
        if (!strcmp(sender_msg,"BOGUS")){
                struct message_2pc_struct new_message;
                new_message.id=node_id;
                new_message.seq=sender_seq;
                new_message.msg="GOT_BOGUS";
		
		packetbuf_copyfrom(&new_message, sizeof(new_message));
  		unicast_send(&uc, from);
		//simple_udp_sendto(&unicast_connection,&new_message,sizeof(new_message), sender_addr);
                
		return;
        }
        if (!strcmp(sender_msg,"REQUEST_COMMIT")){
                struct message_2pc_struct new_message;
                new_message.id=node_id;
                new_message.seq=sender_seq;
                if (state==0){
                        printf("here\n");
                        state=1;
                        new_message.msg="REPLY_COMMIT";
                        printf("sending back positive!! unicast to");
                        //print_ipv6_addr(sender_addr);
                        printf("\n");
                        //simple_udp_sendto(&unicast_connection,&new_message,sizeof(new_message), sender_addr);
			packetbuf_copyfrom(&new_message, sizeof(new_message));
  			unicast_send(&uc, from);
                }
                else{
                        new_message.msg="REPLY_ABORT";
			
			packetbuf_copyfrom(&new_message, sizeof(new_message));
  			unicast_send(&uc, from);
                        //simple_udp_sendto(&unicast_connection,&new_message,sizeof(new_message), sender_addr);
                }
        }
        else{
             state=0;
	     if (!strcmp(sender_msg,"COMMIT")){
		printf("Commiting seq %d from %d\n",sender_seq,sender_id);
	     }
        }


}	


static void
recv_uc(struct unicast_conn *c, const linkaddr_t *from)
{
  printf("unicast message received from %d.%d\n",
         from->u8[0], from->u8[1]);
  printf("unicast message received from %d.%d: '%s'\n",
         from->u8[0], from->u8[1], (char *)packetbuf_dataptr());


	int i;
        struct message_2pc_struct *received_message;
        received_message=packetbuf_dataptr();
        int sender_id=received_message->id;
        int sender_seq=received_message->seq;
        char * sender_msg=received_message->msg;

        if(!strcmp(sender_msg,"GOT_BOGUS")) {
                printf("i got bogus\n");
		return;
        }

        if (sender_seq<saved_seq_numbers[sender_id-1]) {
		printf(" smaller seq number -> ignore\n");
                return;
        }

        printf("unicast message: sender: %d seq: %d message: %s\n",sender_id,sender_seq,sender_msg);

  	//clock_delay(random_rand()*(CLOCK_SECOND/100));
        if(!strcmp(sender_msg,"REPLY_COMMIT")){
                printf("received reply_commit\n");
                votes[sender_id-1]=1;
        }
        else votes[sender_id-1]=2;

        struct message_2pc_struct new_message;

        int flag=0;
        int aborted=0;
        for(i=0;i<num_nodes;i++){
                if (votes[i]==0) flag=1;
                if (votes[i]==2) aborted=1;
        }

        event_bcast_over = process_alloc_event();
        if (aborted==1) {
                printf("ABORT MISSION!\n");
                saved_seq_numbers[node_id-1]++;
		local_seq_number=saved_seq_numbers[node_id-1];
                new_message.id=node_id;
                new_message.seq=sender_seq;
                new_message.msg="ABORT";
                flag_bcast_over_or_aborted=2;
		packetbuf_copyfrom(&new_message, sizeof(new_message));
		broadcast_send(&broadcast);
                //uip_create_linklocal_allnodes_mcast(&addr);
                //simple_udp_sendto(&broadcast_connection,&new_message,sizeof(new_message),&addr);
                process_post(&example_broadcast_process,event_bcast_over,&flag_bcast_over_or_aborted);
        }
        else if (flag==1) printf("STILL WAITING!\n");
        else {
                new_message.id=node_id;
                new_message.seq=sender_seq;
                new_message.msg="COMMIT";
                printf("ALL READY TO COMMIT!\n");
                saved_seq_numbers[node_id-1]++;
		local_seq_number=saved_seq_numbers[node_id-1];
                flag_bcast_over_or_aborted=1;
		packetbuf_copyfrom(&new_message, sizeof(new_message));
		broadcast_send(&broadcast);
                //uip_create_linklocal_allnodes_mcast(&addr);
                //simple_udp_sendto(&broadcast_connection,&new_message,sizeof(new_message),&addr);
                process_post(&example_broadcast_process,event_bcast_over,&flag_bcast_over_or_aborted);
        }



}


static const struct broadcast_callbacks broadcast_call = {broadcast_recv};

static const struct unicast_callbacks unicast_callbacks = {recv_uc};


/*---------------------------------------------------------------------------*/
PROCESS_THREAD(example_broadcast_process, ev, data)
{
  static struct etimer et;
  static struct etimer timeout_timer;
  struct message_2pc_struct bcast_msg;
  int i;

  PROCESS_EXITHANDLER(broadcast_close(&broadcast);)

  PROCESS_BEGIN();
 
  //votes=(int *) malloc(num_nodes*sizeof(int));
  //saved_seq_numbers=(int *) malloc(num_nodes*sizeof(int));
  saved_seq_numbers[node_id -1]=0;	 
  local_seq_number=saved_seq_numbers[node_id-1];
  printf("Node id %d \n",node_id);
  static int dummy=0;
  //simple statistics
  static int succ=0;
  static int aborted=0;
  static int timeout=0;
  static int taken_at_init=0;	 
  static int total=0;
  while(1) {
	total++;
    //if((node_id==5)||(node_id==6)){
	for (i=0;i<num_nodes;i++) votes[i]=0; 
        votes[node_id-1]=1;

    	/* Delay 2-4 seconds */
	dummy++;
	printf("dummy %d \n",dummy);
    	//etimer_set(&et, CLOCK_SECOND * 2 + random_rand() % (CLOCK_SECOND * 2));

    	//PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
	//bcast_msg.id=node_id;
        //bcast_msg.seq=node_seq_number;
        //bcast_msg.msg="BOGUS";
        bcast_msg.id=node_id;
        bcast_msg.seq=saved_seq_numbers[node_id-1];
        char *temp="REQUEST_COMMIT";
        bcast_msg.msg=temp;
        flag_bcast_over_or_aborted=0;

	
	packetbuf_copyfrom(&bcast_msg, sizeof(bcast_msg));
    	if (state==1){
		saved_seq_numbers[node_id-1]++;
		local_seq_number=saved_seq_numbers[node_id-1];
		taken_at_init++;
		printf("Bcast no %d unsuccesfull, backing off and retrying\n",saved_seq_numbers[node_id-1]-1);
    		etimer_set(&et, CLOCK_SECOND * 1 + random_rand() % (CLOCK_SECOND * 1));

    		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
        	continue;
	}
	state=1;
	etimer_set(&timeout_timer,TIMEOUT);
	broadcast_send(&broadcast);
    	printf("broadcast message sent\n");
        PROCESS_WAIT_EVENT_UNTIL((ev == event_bcast_over)||(etimer_expired(&timeout_timer)));
        printf("wait is over\n");
        state=0;
	if (etimer_expired(&timeout_timer)){
		printf(" Bcast no %d timed out, moving on to the next one\n",saved_seq_numbers[node_id-1]);
		timeout++;
        	bcast_msg.id=node_id;
        	bcast_msg.seq=saved_seq_numbers[node_id-1];
        	char *temp="ABORT";
        	bcast_msg.msg=temp;
		packetbuf_copyfrom(&bcast_msg, sizeof(bcast_msg));
		broadcast_send(&broadcast);
		saved_seq_numbers[node_id-1]++;
		local_seq_number=saved_seq_numbers[node_id-1];
		continue;
	}
        //while (       saved_seq_number==node_seq_number) printf("hey\n"); //wait untill bcast is successfull
        int flag= (*(int *)data);
        if ((flag)==1){
                printf(" Bcast no %d was succesfull, moving on to the next \n",saved_seq_numbers[node_id-1]-1);
                        //node_seq_number++;
		succ++;
        }
        else{
                printf("Bcast no %d unsuccesfull, backing off and retrying\n",saved_seq_numbers[node_id-1]-1);
                        //node_seq_number++;
		aborted++;
        }
		printf("Statistics_%d: Total %d, succesfull %d, aborted %d, timedout %d, busy at start %d\n",node_id,
			saved_seq_numbers[node_id-1],succ,aborted,timeout,taken_at_init);
        if ((flag)==1) {
        	event_2pc_succ = process_alloc_event();
                process_post(&start_2pc_process,event_2pc_succ,NULL);
		break;
	}
    //}
    //else{
    //	PROCESS_WAIT_EVENT();
    //	}
   }

  PROCESS_END();
}


PROCESS_THREAD(start_2pc_process, ev, data)
{

  //PROCESS_EXITHANDLER(broadcast_close(&broadcast);)
  static struct etimer et;
  static struct shared_to_comm_message * new_msg;
  PROCESS_BEGIN();
  //broadcast_open(&broadcast, 129, &broadcast_call);
  //unicast_open(&uc, 146, &unicast_callbacks);
  
  etimer_set(&et, CLOCK_SECOND * 2 + random_rand() % (CLOCK_SECOND * 2));
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
  if ((node_id==5)||(node_id==6)){
	new_msg=(struct shared_to_comm_message *)data;
        int w_loc = new_msg->w_loc;
	int w_value = new_msg->w_value;
	printf(" got: wloc -> %d   wvalue %d \n",w_loc,w_value); 
  	process_start(&example_broadcast_process,"START_BCAST");
	//printf("called the other process\n");
	PROCESS_WAIT_EVENT_UNTIL(ev == event_2pc_succ );
	printf("2PC was successful\n");
  }
  //while(1){
//	PROCESS_WAIT_EVENT();
//  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
