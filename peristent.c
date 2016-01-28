#include "contiki.h"
#include "net/rime/rime.h"
#include "random.h"
#include "sys/node-id.h"
#include "dev/button-sensor.h"
#include "shared_variables.h"
#include "test_rime_simple.c"

#include "dev/leds.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>




PROCESS(main_process, "main_process");
AUTOSTART_PROCESSES(&main_process);


static void Shared_Write(int loc, int value){
	
  struct shared_to_comm_message new_msg;
  new_msg.w_loc=loc;
  new_msg.w_value= value;
  new_msg.op = "WRITE"; 
  process_start(&start_2pc_process,&new_msg);
  //start_2pc_process(&new_msg);
	
}

int  Shared_Comp_and_Swap(int loc, int exp_value, int new_value){
  
  struct shared_to_comm_message new_msg;
  new_msg.w_loc=loc;
  new_msg.exp_value=exp_value;
  new_msg.w_value= new_value;
  new_msg.op = "COMP_AND_SWAP"; 
  process_start(&start_2pc_process,&new_msg);
}

PROCESS_THREAD(main_process, ev, data)
{

  PROCESS_BEGIN();
  
  broadcast_open(&broadcast, 129, &broadcast_call);
  unicast_open(&uc, 146, &unicast_callbacks);
  saved_seq_numbers[node_id -1]=0;
 
  Shared_Write(node_id,node_id+5);
  PROCESS_WAIT_EVENT_UNTIL(ev == event_2pc_to_comm );
  printf("ouf\n");
  Shared_Comp_and_Swap(node_id,10,node_id+5);

  PROCESS_WAIT_EVENT_UNTIL(ev == event_2pc_to_comm );
  printf("ouf2\n");
  if(!(strcmp(data,"BCAST_S"))){
 	printf("Comp and Swap was succ\n");
  }
  else{
 	printf("Comp and Swap was not succ\n");
  }
  //process_start(&start_2pc_process,&new_msg);
  //event_start_bcast = process_alloc_event();
  //process_post(&start_2pc_process,event_start_bcast,NULL);
  //process_start(&start_2pc_process,&new_msg);
  //process_start(&start_2pc_process,NULL);
  //process_post(&start_2pc_process,NULL,&new_msg);

  while(1) PROCESS_WAIT_EVENT();


  PROCESS_END();
}

