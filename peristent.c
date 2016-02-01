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



static struct application_struct{
	int load;
	int lower_slot;
	int upper_slot;
};

PROCESS(main_process, "main_process");
PROCESS(schedule_task, "schedule_task");
AUTOSTART_PROCESSES(&main_process);


static void Shared_Write(int loc, int value){
	
  struct shared_to_comm_message new_msg;
  new_msg.w_loc=loc;
  new_msg.w_value= value;
  new_msg.op = "WRITE"; 
  process_start(&start_2pc_process,&new_msg);
  //start_2pc_process(&new_msg);
	
}

static void  Shared_Comp_and_Swap(int loc, int exp_value, int new_value){
  
  struct shared_to_comm_message new_msg;
  new_msg.w_loc=loc;
  new_msg.exp_value=exp_value;
  new_msg.w_value= new_value;
  new_msg.op = "COMP_AND_SWAP"; 
  process_start(&start_2pc_process,&new_msg);
}

PROCESS_THREAD(schedule_task, ev, data)
{	
  static int i;
  static int min;
  static int min_slot;
  static int flag;
  static struct application_struct  new_task;

  PROCESS_BEGIN();
  
  new_task.load=((struct application_struct *)data)->load;
  new_task.lower_slot=((struct application_struct *)data)->lower_slot;
  new_task.upper_slot=((struct application_struct *)data)->upper_slot;
  flag=0;
  do{	
  	min=hourly_load[new_task.lower_slot];
	min_slot=new_task.lower_slot;
	for(i=(new_task.lower_slot+1);i<=new_task.upper_slot;i++){
		if (hourly_load[i]<min){
			min_slot=i;
			min=hourly_load[i];
		}
	}
 	Shared_Comp_and_Swap(min_slot,min,min+new_task.load);
  	PROCESS_WAIT_EVENT_UNTIL(ev == event_2pc_to_comm );
  	if(!(strcmp(data,"BCAST_S"))){
 		printf("Comp and Swap was succ\n");
		flag=1;
  	}
  	else{
 		printf("Comp and Swap was not succ\n");
  	}
  }     
  while(flag==0);

  PROCESS_END();
}


PROCESS_THREAD(main_process, ev, data)
{
  static struct etimer et;
  static int i;
  static struct application_struct  new_task;
  PROCESS_BEGIN();
  
  broadcast_open(&broadcast, 129, &broadcast_call);
  unicast_open(&uc, 146, &unicast_callbacks);
  saved_seq_numbers[node_id -1]=0;
  for(i=0; i<24;i++){
 	//hourly_load[i]=0;
	//state[i]=0;
  }

  etimer_set(&et, CLOCK_SECOND * 2 + random_rand() % (CLOCK_SECOND * 2));
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
  
  new_task.load=5;
  new_task.lower_slot=1;
  new_task.upper_slot=4; 
  process_start(&schedule_task,&new_task);	  

 /*
  Shared_Write(node_id,node_id+5);
  PROCESS_WAIT_EVENT_UNTIL(ev == event_2pc_to_comm );
  printf("ouf\n");

  printf(" load at slot %d is %d\n",node_id,hourly_load[node_id]);
  Shared_Comp_and_Swap(node_id,10,node_id+7);
 
  PROCESS_WAIT_EVENT_UNTIL(ev == event_2pc_to_comm );
  printf("ouf2\n");
  printf(" load at slot %d is %d\n",node_id,hourly_load[node_id]);
  if(!(strcmp(data,"BCAST_S"))){
 	printf("Comp and Swap was succ\n");
  }
  else{
 	printf("Comp and Swap was not succ\n");
  }
  */

  //process_start(&start_2pc_process,&new_msg);
  //event_start_bcast = process_alloc_event();
  //process_post(&start_2pc_process,event_start_bcast,NULL);
  //process_start(&start_2pc_process,&new_msg);
  //process_start(&start_2pc_process,NULL);
  //process_post(&start_2pc_process,NULL,&new_msg);

  

  while(1) PROCESS_WAIT_EVENT();


  PROCESS_END();
}

