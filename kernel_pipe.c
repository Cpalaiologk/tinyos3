
#include "tinyos.h"
#include "kernel_dev.h"
#include "kernel_streams.h"
#include "kernel_cc.h"

int Wnothing(void* this, const char* buf, unsigned int size){ //isws xreiazetai na to dhlwsoyme se .h to balame sto dev.h pros to paron
	return -1; 
}

int Rnothing(void* this, char* buf, unsigned int size){
 return -1; 
}

//ayta den ta exw alla3ei
file_ops read_ops = {
  // .Open = NULL,
  .Read = read_op,
  .Write = Wnothing,
  .Close = r_Close
};

file_ops write_ops = {
  // .Open = NULL,
  .Read = Rnothing,
  .Write = write_op,
  .Close = w_Close
};
//ayta den ta exw alla3ei

int sys_Pipe(pipe_t* pipe)
{
	Fid_t fid[2];
	FCB* fcb[2];

	P_CB* p = (P_CB*) malloc(sizeof(P_CB));
 	p->buf = (char*) malloc(BUFFER_SIZE);


	// Reserve 2 FCBs //mporoyme na to kanoyme me pinaka
	int reserved_fid = FCB_reserve(2, fid, fcb);

	//the available file ids for the process are exhausted
	if(!(reserved_fid))
	{
		fprintf(stderr, "%s\n","exhausted fcb" );
		return -1;
	}

	pipe->read = fid[0];
	pipe->write = fid[1];

	p->is_closed_read = 0;
	p->is_closed_write = 0;
	p->r_index = 0;
	p->w_index = 0;
 	p->empty = COND_INIT;
 	p->full = COND_INIT;

	fcb[0]->streamobj = p;
	fcb[1]->streamobj = p;
	fcb[0]->streamfunc = &read_ops;
	fcb[1]->streamfunc = &write_ops;

	return 0;
}



int read_op(void* t, char *buf, unsigned int size){

	P_CB* p_cb = (P_CB*) t;
	int counter = 0;
	if(p_cb->is_closed_read){ //den to grafei sta errors alla einai logiko
			// fprintf(stderr, "%s\n","GTXSKTPS");
      return -1;
	}

	if(p_cb == NULL){ //den to grafei sta errors alla einai logiko //- The file descriptor is invalid.
			// fprintf(stderr, "%s\n","PWNWS SFEROYLOI");
		return -1;
	}

	while((p_cb->is_closed_read==0) && (p_cb->r_index == p_cb->w_index) && (p_cb->is_closed_write==0)){ 
      //Sto fro eipame oti prwta prepei na 3ypnhsoyme th write (broadcast) an koimatai gia na grapsei! dld ti akribws prepei na kanoyme?
			// fprintf(stderr, "%s\n","While 1");
      kernel_wait(& (p_cb->empty), SCHED_PIPE );
  	}

	//Isws xreiazetai na eleg3oyme an yparxei to fid_t ston pinaka twn fid_ts toy CURPROC gia thn periptwsh toy invalid file descriptor

	//IO runtime problem -> return -1

	// Once a pipe is constructed, it remains operational as long as both
	// ends are open. If the read end is closed, the write end becomes 
	// unusable: calls on @c Write to it return error. On the other hand,
	// if the write end is closed, the read end continues to operate until
	// the buffer is empty, at which point calls to @c Read return 0. 

	if(p_cb->is_closed_write){
			// fprintf(stderr, "%s\n","If 3");
		//mpla mpla mpla mexri na einai empty
  	  //fprintf(stderr, "%s\n", "Write closed." );
        // Read 1 char into buf until the this->buf is empty or buf is full
  		while(p_cb->r_index != p_cb->w_index)
  		{	
  				// fprintf(stderr, "%s\n","While 2");
  			if((counter == size) || (counter == BUFFER_SIZE) ) 
  			{
  					// fprintf(stderr, "%s\n","If 4");
  				return 0;
  			} 
  			buf[counter]= p_cb->buf[p_cb->r_index];
  			p_cb->r_index = (p_cb->r_index + 1) % BUFFER_SIZE; // Circular incr this->read_index
  			counter++; // Incr 
  		}

		return 0;
	}

    // Regular read       // If read_index reaches write_index (buffer is empty): break // If buf_index reaches end: break
  	while(1)
  	{
  			//fprintf(stderr, "%s\n","While 3");
  	  if((p_cb->r_index == p_cb->w_index) && (p_cb->is_closed_write==0)) {
  	  		// fprintf(stderr, "%s\n","If 5");
  	  	break;
  	  }
	  if((counter == size) || (counter == BUFFER_SIZE)){
	  		// fprintf(stderr, "%s\n","If 6");
  	  	break;
  	  }

      // Read 1 char from this->buf[] into buf[]
  	  buf[counter]= p_cb->buf[p_cb->r_index];
  	  p_cb->buf[p_cb->r_index] = '\0'; // Clear element //na to dokimasoyme me '\0'
  	  p_cb->r_index = (p_cb->r_index + 1) % BUFFER_SIZE; // Circular incr this->read_index
  	  counter++; // Incr buf_index
	 }
	if(counter == BUFFER_SIZE){  //EOF -> return 0 Ayto isxyei?
			// fprintf(stderr, "%s\n","If 7");
		return 0;
	}
  	// Signal writer(s)
    kernel_broadcast(& (p_cb->full) );
  	return counter;
}



int write_op(void* t, const char* buf, unsigned int size)
{
//  fprintf(stderr, "%s\n", "IN Write()");
  P_CB* p_cb = (P_CB*) t;
  int counter = 0;

	if(p_cb->is_closed_write==1 || (p_cb->is_closed_read==1)){ //den to grafei sta errors alla einai logiko
	  return -1;
	}

	if(p_cb == NULL){ //den to grafei sta errors alla einai logiko //- The file id is invalid.
		return -1;
	}


	// If write_index reaches read_index (buffer is full): wait for a read     
	while( ((p_cb->w_index + 1) % BUFFER_SIZE == p_cb->r_index) && ((p_cb->is_closed_read)==0))
	{ 
	  //fprintf(stderr, "%s\n", "write() waiting for read()" );    
		kernel_wait(& (p_cb->full), SCHED_PIPE);
	}

	// If write_index reaches read_index (buffer is full): wait for a read // If counter reaches end: break
	while(1) //trollies
	{

	  if(((p_cb->w_index + 1) % BUFFER_SIZE == p_cb->r_index) && (p_cb->is_closed_read==0)){
  	  	break;
  	  }
	  if((counter == size) || (counter == BUFFER_SIZE)){
  	  	break;
  	  }
	 	// Write 1 char from buf[] into this->buf[]
		p_cb->buf[p_cb->w_index] = buf[counter];
		p_cb->w_index = (p_cb->w_index + 1) % BUFFER_SIZE; // Circular incr this->write_index
		counter++; // Incr counter
	}

	// Signal reader(s)
	kernel_broadcast(& (p_cb->empty) );
	return counter;
}


int r_Close(void* streamobj)
{	
	P_CB* p_cb = (P_CB*) streamobj;
  
	if(p_cb!=NULL)
	{    
		p_cb->is_closed_read = 1;
		// Just in case...
		//kernel_broadcast(& pipe_cb->hasSpace); 
		if(p_cb->is_closed_write)
		{
			free(p_cb->buf);
			free(p_cb); 
			p_cb = NULL;
		}
		return 0;
	}
	return -1;
}

int w_Close(void* streamobj)
{
	P_CB* p_cb = (P_CB*) streamobj;

	if(p_cb!=NULL)
	{
		p_cb->is_closed_write = 1;
		// Just in case...
		// kernel_broadcast(& pipe_cb->hasData); 
		if(p_cb->is_closed_read)
		{
			free(p_cb->buf);
			free(p_cb); 
			p_cb = NULL;
		}
		return 0;
	}
	return -1;	
}










