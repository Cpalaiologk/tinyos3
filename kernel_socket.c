
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
		// fprintf(stderr, "%s\n","exhausted fcb" );
		return NOFILE;
	  }
	  SCB* scb = malloc(sizeof(SCB));
  	  scb->ref_counter = 0;
  	  scb->fid = fid[0]; 
  	  scb->fcb = get_fcb(fid[0]);  	 
  	  fcb[0]->streamobj = scb;
  	  fcb[0]->streamfunc = & socket_ops;
  	  
  	  // If port == NOPORT, bound socket to it
  	  if(port != NOPORT){
  	 	 scb->port = port;
  	  }
  	  
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
		// fprintf(stderr, "%s\n","KALHSPERAAAAAAAA" );
		SCB* scb = (SCB*) streamobj;

		if( (scb->s_type == PEER) ){
			int rc = r_Close(scb->peer.receiver); //isws tis kanoyme void giati de mas xreiazontai oi times epistrofhs toys
			int wc = w_Close(scb->peer.sender);
			if(!(rc && wc)){ //isws trollies
		// fprintf(stderr, "%s\n","ifffffffffffffffff" );
				return -1;
			}
		}
		if((scb->s_type == LISTENER)){
			kernel_broadcast(& scb->listener.request_cv); //giati to kanoyme ayto?
		}

		free(scb);
		scb = NULL;
		return 0;
	}
	return -1;
}

int sys_Listen(Fid_t sock)
{
	FCB* fcb = get_fcb(sock);
	if((fcb != NULL) && (fcb->streamfunc =& socket_ops))	{   
		SCB* scb = fcb->streamobj;

		if((scb != NULL) && (scb->port>NOPORT && (scb->port<=MAX_PORT)) && (scb->s_type==UNBOUND) && (((PORT_MAP[scb->port])->s_type)!=LISTENER))
		{
			scb->s_type = LISTENER;
			scb->listener.request_cv = COND_INIT;
			rlnode_init(& scb->listener.queue, NULL);

			return 0;
		}
		//return -1;
	}
	return -1;
}


Fid_t sys_Accept(Fid_t lsock)
{
	FCB* fcb = get_fcb(lsock);

	// fprintf(stderr, "%s %d\n", );

	if(fcb!=NULL && fcb->streamfunc != NULL)
	{	
		SCB* scb = fcb->streamobj;

		if((scb != NULL) && (scb->port>NOPORT && scb->port<=MAX_PORT) && ((PORT_MAP[scb->port])->s_type == LISTENER) && (scb->s_type == LISTENER)) { //na 3anadoyme toys elegxoys
			// fprintf(stderr, "%s\n", "in Accept(), after checks" );
			while(is_rlist_empty(& scb->listener.queue)){
				kernel_wait(& scb->listener.request_cv, SCHED_PIPE);
			}

			Fid_t peer2_fid = sys_Socket(scb->port);
			// fprintf(stderr, "%s %d\n","to scb->port einai -->", scb->port);
			
			if(peer2_fid == NOFILE){
				return NOFILE;
			}

			FCB* fcb = get_fcb(peer2_fid);
			SCB* peer2 = fcb->streamobj;

			// Fetch client_socket_cb from lsock's queue
			rlnode* rl_req_node = rlist_pop_front(& scb->listener.queue);
			reqnode* req_node = rl_req_node->obj;
			SCB* peer1 = req_node->req_from;
			Fid_t peer1_fid = peer1->fid;

			if(peer1 == NULL || peer2 == NULL )
			{	
				//fprintf(stderr, "%s\n", "returning NOFILE (in sys_Accept())" );			
				return NOFILE;
			}


			pipe_t pipe1;
			pipe_t pipe2;
			P_CB* pipe_cb1 = construct_pipe(peer1_fid,peer2_fid,& pipe1);
			P_CB* pipe_cb2 = construct_pipe(peer2_fid,peer1_fid,& pipe2);
			//na to doume 
			// sys_Pipe(& pipe1);
			// sys_Pipe(& pipe2);
/*
			fprintf(stderr, "%s %d\n","to pipe1.read einai -->", pipe1.read);
			fprintf(stderr, "%s %d\n","to pipe1.write einai -->", pipe1.write);
*/
			// FCB* fcb1 = get_fcb(pipe1.read);
			//FCB* fcb3 = get_fcb(pipe1.write);
			// FCB* fcb2 = get_fcb(pipe2.read);

			// fprintf(stderr, "%s %d\n","to fcb->refcount einai -->", fcb->refcount);

			// P_CB* pipe_cb1 = fcb1->streamobj;
			// P_CB* pipe_cb2 = fcb2->streamobj;
			//P_CB* pipe_cb3 = fcb3->streamobj;
			
			/*fprintf(stderr, "%s %d\n","to pipe_cb1 einai -->", pipe_cb1);
			fprintf(stderr, "%s %d\n","to pipe_cb3 einai -->", pipe_cb3);
*/
			if((pipe_cb1 != NULL) && (pipe_cb2 != NULL)) {
				// Convert server socket to PEER
				peer2->s_type = PEER;
				peer2->peer.peer_scb = peer1;
			  	peer2->peer.sender = pipe_cb2;
			  	peer2->peer.receiver = pipe_cb1;

			  	// Convert client socket to PEER
			  	peer1->s_type = PEER;
			  	peer1->peer.peer_scb = peer2;
			  	peer1->peer.sender = pipe_cb1;
			  	peer1->peer.receiver = pipe_cb2;
			}

			req_node->admit = 1;
			//fprintf(stderr, "%s\n", "req_node->accepted==1" );
			kernel_signal(& req_node->cv); //signal???
			//fprintf(stderr, "%s\n", "broadcasted connect cv." );

			// fprintf(stderr, "%s\n", "returning OK from Accept()" );
			return peer2_fid;
		}
		return NOFILE;
	}	
	
	//fprintf(stderr, "%s\n", "returning NOFILE" );
	return NOFILE;
}


int sys_Connect(Fid_t sock, port_t port, timeout_t timeout)
{
	// fprintf(stderr, "%s\n","Mphkame sth synarthsh Connect" );
	FCB* fcb = get_fcb(sock);
	int timeout_not_expired;
	if((fcb!=NULL) && (fcb->streamfunc!=NULL) && (port>NOPORT && port<=MAX_PORT) ) {
		SCB* client1 = fcb->streamobj;
		SCB* listener = PORT_MAP[port];
		// fprintf(stderr, "%s\n","Mphkame sthn prwth if" );

		if((client1->s_type == UNBOUND) && (listener!=NULL) && (PORT_MAP[port]->s_type == LISTENER)) {
			// fprintf(stderr, "%s\n","Mphkame sthn if poy gyrnaei 0" );
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
					// fprintf(stderr, "%s\n", "fail mesa sthn if" );
					return -1;
				}
				// else{
				// 	kernel_wait(& req_node->cv, SCHED_PIPE); //giati????
				// }

			}
			free(req_node);
			req_node = NULL;
			return 0;
		}
		// fprintf(stderr, "%s\n","bghkame apo thn prwth if" );

	}
	return -1;
}


int sys_ShutDown(Fid_t sock, shutdown_mode how)
{
	FCB* fcb = get_fcb(sock);

	if((fcb!=NULL) && (fcb->streamfunc==& socket_ops) && (how>=1 && how<=3) )
	{
		SCB* scb = fcb->streamobj;

		if((scb!=NULL) && (scb->s_type == PEER))
		{	
			if(how == 1){
				return r_Close(scb->peer.receiver); 
			}
		  	else if(how == 2){
		   		return w_Close(scb->peer.sender);  
		    }
		    else if(how == 3){
			   	int ret1 = r_Close(scb->peer.receiver);
			    int ret2 = w_Close(scb->peer.sender);

			    if((ret1 == 1) && (ret2 == 1)){
			    	return 0;
			    }
			    else{
			    	return -1;
			    }
		    }
		}
	}

	return -1;
}

