
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
	if ( (port >= NOPORT) && (port <= MAX_PORT) )
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
  	  scb->fcb = get_fcb(fid[0]);  	 
  	  fcb[0]->streamobj = scb;
  	  fcb[0]->streamfunc = & socket_ops;
  	  
  	  // If port == NOPORT, bound socket to it
  	  scb->port = port;
  	  
  	  scb->s_type = UNBOUND;
  	  //SCB->unbound_socket.req_node = NULL; //request node initialize
	  if(PORT_MAP[port] == NULL){
    	  	PORT_MAP[port] = scb;
    	}
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
	FCB* fcb = get_fcb(sock);
	int timeout_not_expired;
	if((fcb->streamfunc!=NULL) && (port>NOPORT && port<=MAX_PORT) ) {
		SCB* client1 = fcb->streamobj;
		SCB* listener = PORT_MAP[port];

		if((client1->s_type == UNBOUND) && (listener->s_type == LISTENER)){
			reqnode* req_node = malloc(sizeof(reqnode));
			req_node->req_from = client1;
		  	rlnode_init(& req_node->rl_node, req_node);
		  	req_node->cv = COND_INIT;
		  	req_node->admit = 0;
			// Push back
			rlist_push_back(& (listener->listener.queue), & (req_node->rl_node) );
			kernel_signal(& listener->listener.request_cv); //broadcast??
			while(req_node->admit == 0) {	
			//	if(timeout>=0)
			//	{	
				//fprintf(stderr, "%s\n", "in Connect() while");
				timeout_not_expired = kernel_timedwait(& req_node->cv, SCHED_PIPE, timeout);

				if(!timeout_not_expired) //an den egine expired rip
				{
					fprintf(stderr, "%s\n", "returning -1 from Connect() 1" );
					return -1;
				}
			//	}
			//	else
			//		kernel_wait(& req_node->cv, SCHED_PIPE);

			}
			// free(req_node);
			// req_node = NULL;
			return 0;
		}

/*
			free(req_node);
			req_node = NULL;*/
			// return 0;
	}
	return -1;
}


int sys_ShutDown(Fid_t sock, shutdown_mode how)
{
	return -1;
}

