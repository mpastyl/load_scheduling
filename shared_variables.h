PROCESS_NAME(main_process);
PROCESS_NAME(start_2pc_process);
PROCESS_NAME(example_broadcast_process);

static struct unicast_conn uc;
static struct broadcast_conn broadcast;

static int hourly_load[24];

static int naive_counter=0;
static int votes[6];
static int saved_seq_numbers[6];
static int num_nodes=6;
static int state=0; //state=1 we are in phase 1 of some broadcast so reject new incoming broadcasts
//int node_seq_number=0;  
static int flag_bcast_over_or_aborted=0;


//
static int local_seq_number=0;


static struct shared_to_comm_message{
	int w_loc;
	int w_value;
}; 