
#include "tinyos.h"
#include "kernel_sched.h"
#include "kernel_proc.h"

/** 
  @brief Create a new thread in the current process.
  */
//Ξεκινά νέο thread, με attributes a (μπορεί να είναι NULL) εκτελώντας την func με όρισμα arg. Το thread id αποθηκεύεται στο t.
Tid_t sys_CreateThread(Task task, int argl, void* args)
{
  // Shmeiwseis vsam apo diale3eis. func = task (?), t = Tid_t (?) Task einai pointer se synarthsh
  // int pthread_create(t, a, func, arg)‏
  // pthread_t* t;
  // const pthread_attr_t* a
  // void* (*func)(void*);
  // void* arg;

	return NOTHREAD;
}

/**
  @brief Return the Tid of the current thread.
 */
Tid_t sys_ThreadSelf()
{
	return (Tid_t) CURTHREAD; //Einai etoimh de xreiazetai na kanoyme kati "Επιστρέφει το id του τρέχοντος thread."" 8elw na epistrefei pointer cast se akeraio toy ptcb giati einai pio eykolo na doyleyw me ptcb
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
	return -1;
}

/**
  @brief Detach the given thread.
  */
//pote kanoyme detach??
int sys_ThreadDetach(Tid_t tid)
{
	return -1;
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
}

