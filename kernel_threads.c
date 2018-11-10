
#include "tinyos.h"
#include "kernel_sched.h"
#include "kernel_proc.h"



void start_PTCB_thread()
{ 
  int exitval;

  Task call =  CURPTHREAD->main_task;
  int argl = CURPTHREAD->argl;
  void* args = CURPTHREAD->args;
  exitval = call(argl,args);
 
  // allagmeno
  ThreadExit(exitval);  
}


PTCB* create_PTCB(Task task, int argl, void* args, PCB* pcb)
{
    PTCB* ptcb = (PTCB*) xmalloc(sizeof(PTCB));

    /* Push back to CURPROC->PTCB_list */
    rlnode_init(& ptcb->PTCB_node, ptcb); 


    rlist_push_back(& (pcb->PTCB_list), & ptcb->PTCB_node); //

     /* Initialize PTCB attrs*/
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

    return  ptcb;
}


/** 
  @brief Create a new thread in the current process.
  */
//Ξεκινά νέο thread, με attributes a (μπορεί να είναι NULL) εκτελώντας την func με όρισμα arg. Το thread id αποθηκεύεται στο t.
// If this thread returns from function task, the return value is used as an argument to  `ThreadExit`.
Tid_t sys_CreateThread(Task task, int argl, void* args)
{
    TCB* tcb;
    PTCB* ptcb = create_PTCB(task, argl, args, CURPROC);

    if(task!=NULL)
    {
      tcb = spawn_thread(CURPROC, start_PTCB_thread);
      ptcb->master_thread=tcb; 
      ptcb->master_thread->owner_ptcb = ptcb;
    }
    else{
        return NOTHREAD; //on success epistrefei to thread id on fail epistrefei nothread
    }

    wakeup(ptcb->master_thread);

    return (Tid_t) ptcb;
}

/**
  @brief Return the Tid of the current thread.
 */
Tid_t sys_ThreadSelf()
{
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

  // PCB* thisproc=CURPROC;

  PTCB* ptcb = (PTCB*)tid;

  if(rlist_find(&CURPROC->PTCB_list,ptcb,NULL)!=NULL){ // - there is no thread with the given tid in this process.
      return -1;  
  }

  if(ptcb==CURTHREAD->owner_ptcb){     //- the tid corresponds to the current thread
    return -1;
  }

  //Is detached?
  if(ptcb->is_detached==1){            //- the tid corresponds to a detached thread.
    return -1;
  }     
  if(exitval!=NULL){//Save exit value
    *exitval=ptcb->exitval; //
    return 0;
  }
	return -1;
}

/**
  @brief Detach the given thread.
  */
//pote kanoyme detach??
int sys_ThreadDetach(Tid_t tid)
{

  PTCB* ptcb = (PTCB*)tid;

  if(rlist_find(&CURPROC->PTCB_list,ptcb,NULL)!=NULL){ // - there is no thread with the given tid in this process.
      return -1;  
  }

  if(ptcb->is_exited==1){                              // - the tid corresponds to an exited thread.
    return -1;
  }

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

   // fprintf(stderr, "%s%lu\n","in ThreadExit with ptcb tid:", (Tid_t) CURPTHREAD );
  CURPTHREAD->exitval = exitval;
  CURPTHREAD->is_exited = 1;
  kernel_broadcast(& CURPTHREAD->cv);
  // fprintf(stderr, "%s\n", "in ThreadExit() after kernel_broadcast(), before kernel_sleep()" );
  // TODO-maybe: if(DETACHED): kernel_sleep() and delete_PTCB()
  kernel_sleep(EXITED, SCHED_USER);

}

