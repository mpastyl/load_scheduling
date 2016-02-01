
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

#include "shared_variables.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*---------------------------------------------------------------------------*/
PROCESS(example_broadcast_process, "Broadcast example");
PROCESS(start_2pc_process, "start_2pc_process");
//AUTOSTART_PROCESSES(&start_2pc_process);
/*---------------------------------------------------------------------------*/

#define TIMEOUT		(4*CLOCK_SECOND)



static process_event_t event_bcast_over;
static process_event_t event_2pc_succ;

static struct shared_to_comm_message  brand_new_msg;

struct message_2pc_struct{
        int id;
        int seq;
        char * msg;
	int w_loc;
	int w_value;
	char * op;
	int exp_value;
};


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
  char * sender_op = received_message->op;
  int sender_wloc = received_message->w_loc;
  int sender_wvalue = received_message->w_value;
  int sender_exp_value = received_message->exp_value;
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
                
		return;
        }
        if (!strcmp(sender_msg,"REQUEST_COMMIT")){
                struct message_2pc_struct new_message;
                new_message.id=node_id;
                new_message.seq=sender_seq;
		new_message.op=sender_op;
		new_message.w_loc=sender_wloc;
		new_message.w_value=sender_wvalue;
		new_message.exp_value=sender_exp_value;
                if (state[sender_wloc]==0){
			if(strcmp(sender_op,"COMP_AND_SWAP")||((!strcmp(sender_op,"COMP_AND_SWAP"))&&(hourly_load[sender_wloc]==sender_exp_value))){ 
                        	state[sender_wloc]=1;
                        	new_message.msg="REPLY_COMMIT";
                        	printf("sending back positive!! unicast to");
                        	printf("\n");
				packetbuf_copyfrom(&new_message, sizeof(new_message));
  				unicast_send(&uc, from);
			}
			else{
                        	new_message.msg="REPLY_ABORT";
				printf("Aborting because of conflicting values\n");
				packetbuf_copyfrom(&new_message, sizeof(new_message));
				unicast_send(&uc, from);
			}
                }
                else{
                        new_message.msg="REPLY_ABORT";
			
			packetbuf_copyfrom(&new_message, sizeof(new_message));
  			unicast_send(&uc, from);
                }
        }
        else{
	     if (!strcmp(sender_msg,"COMMIT")){

		//if (!strcmp(received_message->op,"WRITE")){
		hourly_load[sender_wloc] = sender_wvalue;
		printf("Commiting  write seq %d from %d: location %d value %d\n",sender_seq,sender_id,sender_wloc,sender_wvalue);
		//}
	     }
             state[sender_wloc]=0;
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
        static struct message_2pc_struct *received_message;
        received_message=packetbuf_dataptr();
        int sender_id=received_message->id;
        int sender_seq=received_message->seq;
        char * sender_msg=received_message->msg;
  	char * sender_op = received_message->op;
  	int sender_wloc = received_message->w_loc;
  	int sender_wvalue = received_message->w_value;
  	int sender_exp_value = received_message->exp_value;
	

        if(!strcmp(sender_msg,"GOT_BOGUS")) {
                printf("i got bogus\n");
		return;
        }

        if (sender_seq<saved_seq_numbers[node_id-1]) {
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
		printf("seq number is now %d\n",saved_seq_numbers[node_id-1]);
		local_seq_number=saved_seq_numbers[node_id-1];
                new_message.id=node_id;
                new_message.seq=sender_seq;
                new_message.msg="ABORT";
		new_message.op=sender_op;
		new_message.w_loc=sender_wloc;
		new_message.w_value=sender_wvalue;
		new_message.exp_value=sender_exp_value;

                flag_bcast_over_or_aborted=2;
		packetbuf_copyfrom(&new_message, sizeof(new_message));
		broadcast_send(&broadcast);
                process_post(&example_broadcast_process,event_bcast_over,&flag_bcast_over_or_aborted);
        }
        else if (flag==1) printf("STILL WAITING!\n");
        else {
                new_message.id=node_id;
                new_message.seq=sender_seq;
                new_message.msg="COMMIT";
		new_message.op=sender_op;
		new_message.w_loc=sender_wloc;
		new_message.w_value=sender_wvalue;
		new_message.exp_value=sender_exp_value;
		// at this version, coordinator participates in its own commit
		// have already checked if its a valid comp and swap
		hourly_load[new_message.w_loc] = new_message.w_value;
                printf("ALL READY TO COMMIT!\n");
                saved_seq_numbers[node_id-1]++;
		local_seq_number=saved_seq_numbers[node_id-1];
                flag_bcast_over_or_aborted=1;
		packetbuf_copyfrom(&new_message, sizeof(new_message));
		broadcast_send(&broadcast);
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
  static struct message_2pc_struct bcast_msg;
  int i;
  static struct shared_to_comm_message target_msg;
  PROCESS_EXITHANDLER(broadcast_close(&broadcast);)

  PROCESS_BEGIN();
 
  target_msg.w_loc= ((struct shared_to_comm_message *)data)->w_loc;
  target_msg.w_value= ((struct shared_to_comm_message *)data)->w_value;
  target_msg.op= ((struct shared_to_comm_message *)data)->op;
  target_msg.exp_value= ((struct shared_to_comm_message *)data)->exp_value;
  printf("trying to commit %d   %d \n",target_msg.w_loc, target_msg.w_value);
  //votes=(int *) malloc(num_nodes*sizeof(int));
  //saved_seq_numbers=(int *) malloc(num_nodes*sizeof(int));
  //saved_seq_numbers[node_id -1]=0;	 
  local_seq_number=saved_seq_numbers[node_id-1];
  printf("Node id %d \n",node_id);
  //simple statistics
  static int succ=0;
  static int aborted=0;
  static int timeout=0;
  static int taken_at_init=0;	 
  static int total=0;

  static int flag=0;
  while(1) {
	total++;
    //if((node_id==5)||(node_id==6)){
	for (i=0;i<num_nodes;i++) votes[i]=0; 
        votes[node_id-1]=1;

        bcast_msg.id=node_id;
        bcast_msg.seq=saved_seq_numbers[node_id-1];
        char *temp="REQUEST_COMMIT";
        bcast_msg.msg=temp;
	bcast_msg.op=target_msg.op;
	bcast_msg.w_loc=target_msg.w_loc;
	bcast_msg.w_value=target_msg.w_value;
	bcast_msg.exp_value=target_msg.exp_value;
        
 	flag_bcast_over_or_aborted=0;

	
    	if (state[bcast_msg.w_loc]==1){
		saved_seq_numbers[node_id-1]++;
		local_seq_number=saved_seq_numbers[node_id-1];
		taken_at_init++;
		printf("Bcast no %d unsuccesfull, backing off and retrying\n",saved_seq_numbers[node_id-1]-1);
    		etimer_set(&et, CLOCK_SECOND * 1 + random_rand() % (CLOCK_SECOND * 1));

    		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
        	continue;
	}
	state[bcast_msg.w_loc]=1;
	//First check that the coordinator can commit
	//At this version the coordinator also participates in the commit
        if (!strcmp(target_msg.op,"COMP_AND_SWAP")&&(hourly_load[target_msg.w_loc]!=target_msg.exp_value)){
		printf("Could not comp_and_swap my own location\n");
        	event_2pc_succ = process_alloc_event();
                process_post(&start_2pc_process,event_2pc_succ,"2PC_F");
		break;
		
	}
	////
	etimer_set(&timeout_timer,TIMEOUT);
	packetbuf_copyfrom(&bcast_msg, sizeof(bcast_msg));
	broadcast_send(&broadcast);
        PROCESS_WAIT_EVENT_UNTIL((ev == event_bcast_over)||(etimer_expired(&timeout_timer)));
        printf("wait is over\n");
        //Something fishy going on here, bcast_msg doesnt work even thought static, target_msg works
	state[target_msg.w_loc]=0;
	printf("Hey2: unlocked loc %d\n",target_msg.w_loc);
	if (etimer_expired(&timeout_timer)){
		printf(" Bcast no %d timed out, moving on to the next one\n",saved_seq_numbers[node_id-1]);
		timeout++;
        	bcast_msg.id=node_id;
        	bcast_msg.seq=saved_seq_numbers[node_id-1];
        	char *temp="ABORT";
        	bcast_msg.msg=temp;
		bcast_msg.op=target_msg.op;
		bcast_msg.exp_value=target_msg.exp_value;
		packetbuf_copyfrom(&bcast_msg, sizeof(bcast_msg));
		broadcast_send(&broadcast);
		saved_seq_numbers[node_id-1]++;
		local_seq_number=saved_seq_numbers[node_id-1];
    		etimer_set(&et, CLOCK_SECOND * 1 + random_rand() % (CLOCK_SECOND * 1));

    		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
		continue;
	}
        flag= (*(int *)data);
        if ((flag)==1){
                printf(" Bcast no %d was succesfull, moving on to the next \n",saved_seq_numbers[node_id-1]-1);
		succ++;
        }
        else{
                printf("Bcast no %d unsuccesfull, backing off and retrying\n",saved_seq_numbers[node_id-1]-1);
		aborted++;
		printf("HEY!!!! %s\n",bcast_msg.op);
		if (!strcmp(bcast_msg.op,"COMP_AND_SWAP")){
			printf("I should quit now\n");
        		event_2pc_succ = process_alloc_event();
                	process_post(&start_2pc_process,event_2pc_succ,"2PC_F");
			break;
		}
    		etimer_set(&et, CLOCK_SECOND * 1 + random_rand() % (CLOCK_SECOND * 1));

    		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
        }
	
	printf("Statistics_%d: Total %d, succesfull %d, aborted %d, timedout %d, busy at start %d\n",node_id,
			saved_seq_numbers[node_id-1],succ,aborted,timeout,taken_at_init);
        if ((flag)==1) {
		printf("2pc succesfull??\n");
        	event_2pc_succ = process_alloc_event();
                process_post(&start_2pc_process,event_2pc_succ,"2PC_S");
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
  PROCESS_BEGIN();
  
  brand_new_msg.w_loc=((struct shared_to_comm_message *) data)->w_loc;
  brand_new_msg.w_value=((struct shared_to_comm_message *) data)->w_value;
  brand_new_msg.op=((struct shared_to_comm_message *) data)->op;
  brand_new_msg.exp_value= ((struct shared_to_comm_message *) data)->exp_value;
  
  //etimer_set(&et, CLOCK_SECOND * 2 + random_rand() % (CLOCK_SECOND * 2));
  //PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
  if ((node_id==5)||(node_id==6)){
  	printf(" got: wloc -> %d   wvalue %d \n",brand_new_msg.w_loc,brand_new_msg.w_value); 
  	process_start(&example_broadcast_process,&brand_new_msg);
	//printf("called the other process\n");
	PROCESS_WAIT_EVENT_UNTIL(ev == event_2pc_succ );
	if(!strcmp(data,"2PC_S")){
		printf("2PC was successful\n");
  		event_2pc_to_comm = process_alloc_event();
        	process_post(&schedule_task,event_2pc_to_comm,"BCAST_S");
	}
	else{
		printf("2PC was unsuccessful because a conflict was detected and you wanted cmp_and_swap\n");
  		event_2pc_to_comm = process_alloc_event();
        	process_post(&schedule_task,event_2pc_to_comm,"BCAST_F");
	}
  }
  //while(1){
//	PROCESS_WAIT_EVENT();
//  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
