
#include "tinyos.h"
#include "kernel_sched.h"
#include "kernel_proc.h"

#include "kernel_cc.h"



void start_PTCB_thread()
{ 
  int exitval;

  Task call =  CURPTHREAD->main_task;
  int argl = CURPTHREAD->argl;
  void* args = CURPTHREAD->args;

  // fprintf(stderr, "to call einai : ---------> %d\n", call);
  // fprintf(stderr, "to argl einai : ---------> %d\n", argl);
  // fprintf(stderr, "to args einai : ---------> %d\n", args);
  exitval = call(argl,args);
  // fprintf(stderr, "%s %d\n", "TO EXITVAL EINAIiiiiiiiiiiiiiiiiiiiii : --------->", exitval );

  ThreadExit(exitval);  
}


PTCB* create_PTCB(Task task, int argl, void* args, PCB* pcb)
{

    PTCB* ptcb = (PTCB*) xmalloc(sizeof(PTCB));

    //push back to PCB's PTCB_list
    rlnode_init(& ptcb->PTCB_node, ptcb); 

    rlist_push_back(& (pcb->PTCB_list), & ptcb->PTCB_node); //

     // PTCB INIT
    ptcb->master_thread = NULL;
    ptcb->main_task = task;
    ptcb->owner_pcb = pcb;
    ptcb->exitval = 0; // Isws 8elei na to kanoyme pointer || exitval a location where to store the exit value of the joined 
              //                                                 thread. If NULL, the exit status is not returned.
    ptcb->is_detached = 0; //not detached -> JOINABLE
    ptcb->cv = COND_INIT;
    ptcb->is_exited = 0;
    ptcb->argl = argl;
    ptcb->args = args;

    // fprintf(stderr, "CREATE_PTCB with addr: ----> %d\n", ptcb );
    return  ptcb;
}


/** 
  @brief Create a new thread in the current process.
  */
//Ξεκινά νέο thread, με attributes a (μπορεί να είναι NULL) εκτελώντας την func με όρισμα arg. Το thread id αποθηκεύεται στο t.
// If this thread returns from function task, the return value is used as an argument to  `ThreadExit`.
Tid_t sys_CreateThread(Task task, int argl, void* args)
{   


    // fprintf(stderr, "CREATE THREAD START\n" );
    TCB* tcb;
    PTCB* ptcb = create_PTCB(task, argl, args, CURPROC);

    if(task!=NULL)
    {
      // fprintf(stderr, "IF THS CREATE THREAD ARXH\n" );
      tcb = spawn_thread(CURPROC, start_PTCB_thread);
      ptcb->master_thread=tcb; 
      ptcb->master_thread->owner_ptcb = ptcb;
      // fprintf(stderr, "IF THS CREATE THREAD TELOS\n" );

    }
    else{
        return NOTHREAD; //on success epistrefei to thread id on fail epistrefei nothread
    }

    wakeup(ptcb->master_thread);

    // fprintf(stderr, "CREATE THREAD END\n" );
    return (Tid_t) ptcb;
}

/**
  @brief Return the Tid of the current thread.
 */
Tid_t sys_ThreadSelf()
{

  // fprintf(stderr, "%s\n", "EDW LOGIKA DEN MPAINEI POTE GIATI DEN THN KALOYME POTE LUL\n" );
	return (Tid_t) CURTHREAD->owner_ptcb; //"Επιστρέφει το id του τρέχοντος thread."
  // 8elw na epistrefei pointer cast se akeraio toy ptcb giati einai pio eykolo na doyleyw me ptcb
}

/**
  @brief Join the given thread.
  */
//Περιμένει το t να τερματίσει, αποθηκεύει στο vp την τιμή επιστροφής. 
//Κανονικά για κάθε τερματισμένο thread καλείται η pthread_join
//Αλλιώς έχουμε διαρροή πόρων (κυρίως μνήμης).
//Εναλλακτικά:  detached threads  (aka. daemon threads)
//Σε ένα detached thread δεν πρέπει (και δεν χρειάζεται) να κληθεί η join.
int sys_ThreadJoin(Tid_t tid, int* exitval)
{
  // int pthread_join(t, vp)‏
  // pthread_t* t;
  // void** vp;

  // fprintf(stderr, "%s %d\n", "THREAD JOIN START exitval value is: -----> ", exitval);
  PTCB* ptcb = (PTCB*)tid;

// Mutex_Lock(&CURPROC->ptcb_mx);

    // Spontaneous-wake-up protection loop


  if(rlist_find(&CURPROC->PTCB_list,ptcb,NULL)==NULL){ // - there is no thread with the given tid in this process.
    // Mutex_Unlock(&CURPROC->ptcb_mx);
    return -1;  
  }


  // fprintf(stderr, "%s\n", "THREAD JOIN IF 1 passed\n" );
  if(ptcb==CURTHREAD->owner_ptcb){     //- the tid corresponds to the current thread
    // Mutex_Unlock(&CURPROC->ptcb_mx);
    return -1;
  }

  // fprintf(stderr, "%s\n", "THREAD JOIN IF 2 passed\n" );
  //Is detached?
  if(ptcb->is_detached==1){            //- the tid corresponds to a detached thread.
    // Mutex_Unlock(&CURPROC->ptcb_mx);
    return -1;
  }

  while( (ptcb->is_exited!=1) ){
    // fprintf(stderr, "%s\n", "in JOIN() before kernel_wait()");
    kernel_wait(& ptcb->cv, SCHED_USER); // ???
    // fprintf(stderr, "%s\n", "in JOIN() after kernel_wait()");
  }
 // Cond_Wait(&CURPROC->ptcb_mx,&ptcb->cv);//This line takes forever to execute

  if(exitval!=NULL){//Save exit value
    *exitval=ptcb->exitval; //
    // Mutex_Unlock(&CURPROC->ptcb_mx);
    return 0;
  }


  // Isws 8elei if is exited kai na kanoyme mesa release to thread ama mpei
  // Mutex_Unlock(&CURPROC->ptcb_mx);
  // fprintf(stderr, "%s\n", "THREAD JOIN IF 3 passed\n" );
	return -1;
}

/**
  @brief Detach the given thread.
  */
//pote kanoyme detach??
int sys_ThreadDetach(Tid_t tid)
{

  PTCB* ptcb = (PTCB*)tid;

  // fprintf(stderr, "%s\n", "THREAD DETACH START\n" );
  if(rlist_find(&CURPROC->PTCB_list,ptcb,NULL)!=NULL){ // - there is no thread with the given tid in this process.
      return -1;  
  }

  // fprintf(stderr, "%s\n", "THREAD DETACHED IF 1 passed\n" );
  if(ptcb->is_exited==1){                              // - the tid corresponds to an exited thread.
    return -1;
  }

  // fprintf(stderr, "%s\n", "THREAD DETACHED IF 2 passed\n" );
  ptcb->is_detached=1;

	return 0;
}

/**
  @brief Terminate the current thread.
  */
//Τερματίζει το τρέχον thread επιστρέφοντας val.
// Τερματισμός διεργασίας: 
// όταν τερματίσει το main thread (αυτό που εκτελεί τη main()).
// όταν κληθεί η exit() από κάποιο thread
void sys_ThreadExit(int exitval)
{
  //int pthread_exit(val)‏
  //void* val;

  // fprintf(stderr, "%s\n", "THREAD EXIT START\n" );
   // fprintf(stderr, "%s%lu\n","in ThreadExit with ptcb tid:", (Tid_t) CURPTHREAD );
  CURPTHREAD->exitval = exitval;
  CURPTHREAD->is_exited = 1;
  kernel_broadcast(& CURPTHREAD->cv);
  // fprintf(stderr, "%s\n", "in ThreadExit() after kernel_broadcast(), before kernel_sleep()" );
  // TODO-maybe: if(DETACHED): kernel_sleep() and delete_PTCB()
  kernel_sleep(EXITED, SCHED_USER);

  // fprintf(stderr, "%s\n", "THREAD EXIT END\n" );
}

