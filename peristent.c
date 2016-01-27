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

PROCESS_THREAD(main_process, ev, data)
{

  struct shared_to_comm_message new_msg;
  PROCESS_BEGIN();
  
  broadcast_open(&broadcast, 129, &broadcast_call);
  unicast_open(&uc, 146, &unicast_callbacks);

  new_msg.w_loc=node_id;
  new_msg.w_value= node_id+5;

  process_start(&start_2pc_process,&new_msg);
  //event_start_bcast = process_alloc_event();
  //process_post(&start_2pc_process,event_start_bcast,NULL);
  //process_start(&start_2pc_process,&new_msg);
  //process_start(&start_2pc_process,NULL);
  //process_post(&start_2pc_process,NULL,&new_msg);

  while(1) PROCESS_WAIT_EVENT();


  PROCESS_END();
}

