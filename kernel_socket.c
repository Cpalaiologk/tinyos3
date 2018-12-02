
#include "tinyos.h"
#include "kernel_dev.h"
#include "kernel_streams.h"
#include "kernel_sched.h"
#include "kernel_cc.h"
/* =============SCB==========================*/







/*file_ops socket_ops = {
  .Read = socket_ops_Read,
  .Write = socket_ops_Write,
  .Close = socket_ops_Close
};*/


/*typedef struct request_node
{
  Socket_CB* socket_ptr;
  rlnode rl_node; // Intrusive node
  CondVar cv;
  int accepted; // listener accepted flag
} requestnode;*/

/* ==========================================*/

SCB* PORT_MAP[MAX_PORT + 1]; 


Fid_t sys_Socket(port_t port)
{
      /*	if(port != 0)
	  {
		PORT_MAP[port] = NULL;
	  }
	  else{
	 	PORT_MAP[port] = NOPORT;
	  }*/
	if ( (port > NOPORT) && (port <= MAX_PORT) )
	{
	  Fid_t fid[1];
	  FCB* fcb[1];

	  // Reserve 2 FCBs //mporoyme na to kanoyme me pinaka
	  int reserved_fid = FCB_reserve(1, fid, fcb);

	  //the available file ids for the process are exhausted
	  if(!(reserved_fid))
	  {
		fprintf(stderr, "%s\n","exhausted fcb" );
		return NOFILE;
	  }
	  SCB* scb = malloc(sizeof(SCB));
  	  scb->ref_counter = 0;
  	  scb->closed = 0; 
  	  scb->fcb = fid[0];  	 
  	  fcb[0]->streamobj = scb;
  	  fcb[0]->streamfunc = & socket_ops;
  	  
  	  // If port == NOPORT, bound socket to it
  	  scb->port = port;
  	  
  	  scb->socket_type = UNBOUND;
  	  //SCB->unbound_socket.req_node = NULL; request node initialize

  	  return fid[0];

	}
	return NOFILE;
}

int sys_Listen(Fid_t sock)
{
	return -1;
}


Fid_t sys_Accept(Fid_t lsock)
{
	return NOFILE;
}


int sys_Connect(Fid_t sock, port_t port, timeout_t timeout)
{
	return -1;
}


int sys_ShutDown(Fid_t sock, shutdown_mode how)
{
	return -1;
}

