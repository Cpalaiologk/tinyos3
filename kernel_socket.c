
#include "tinyos.h"
#include "kernel_dev.h"
#include "kernel_streams.h"
#include "kernel_sched.h"
#include "kernel_cc.h"
/* =============SCB==========================*/



file_ops socket_ops = {
  .Read = s_op_Read,
  .Write = s_op_Write,
  .Close = s_op_Close
};


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
  	  scb->fcb = get_fcb(fid[0]);  	 
  	  fcb[0]->streamobj = scb;
  	  fcb[0]->streamfunc = & socket_ops;
  	  
  	  // If port == NOPORT, bound socket to it
  	  scb->port = port;
  	  
  	  scb->s_type = UNBOUND;
  	  //SCB->unbound_socket.req_node = NULL; request node initialize

  	  return fid[0];

	}
	return NOFILE;
}

int s_op_Read(void* socket, char *buf, unsigned int size){
	SCB* scb = (SCB*) socket;

	if((scb->s_type == PEER) && (scb->peer.receiver != NULL))
	{
		return read_op(scb->peer.receiver, buf, size);
	}
	else{
		return -1;
	}
}
int s_op_Write(void* socket,const char *buf,  unsigned int size){
	SCB* scb = (SCB*) socket;

	if((scb->s_type == PEER) && (scb->peer.sender != NULL))
	{
		return write_op(scb->peer.sender, buf, size);
	}
	else{
		return -1;
	}
}
int s_op_Close(void* streamobj){ //aythn k merikes alles isws tis kanoyme void
	if(streamobj!=NULL){
		SCB* scb = (SCB*) streamobj;

		if( (scb->s_type == PEER) ){
			r_Close(scb->peer.receiver); //isws tis kanoyme void giati de mas xreiazontai oi times epistrofhs toys
			w_Close(scb->peer.sender);
		}
		//if((socket_cb->socket_type == LISTENER))
			// kernel_broadcast(& socket_cb->listener_socket.cv_reqs); //giati to kanoyme ayto?

		scb->closed = 1;
		free(scb);
		scb = NULL;
		return 0;
	}
	return -1;
}

int sys_Listen(Fid_t sock)
{
	FCB* fcb = get_fcb(sock);
	if(fcb->streamfunc != NULL)	{   
		SCB* scb = fcb->streamobj;

		if((scb->port>NOPORT && (scb->port<=MAX_PORT)) && (scb->s_type==UNBOUND) && (((PORT_MAP[scb->port])->s_type)!=LISTENER))
		{
			scb->s_type = LISTENER;
			scb->listener.request_cv = COND_INIT;
			rlnode_init(& scb->listener.queue, NULL);

			return 0;
		}
	}
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

