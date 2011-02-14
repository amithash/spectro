#ifndef _QUEUE_H_
#define _QUEUE_H_

/*==============================================================
 *   			    QUEUE
 *
 * This is a simple blocking queue implementation. 
 *
 * Functions:
 * q_init	=	Initialize the queue
 * q_put	=	Enqueues data
 * q_get	=	Dequeues data
 * q_destroy	=	Destroys (de-inits) the queue
 *
 * There is also a non-blocking varient of q_get aptly named:
 * q_get_nonblocking	Dequeues data, but does not block if
 * 			the queue is empty.
 *
 * Key aspects:
 * 1.	SMP Safe (Implemented using pthreads)
 * 2.	Blocking.
 *=============================================================*/
#include <pthread.h>

/****************************************************************
 * 		Elementry data structs
 ***************************************************************/

typedef struct link_struct link_type; /* Internal */

struct link_struct                    /* Internal */
{
	void      *data;
	link_type *next;
};

typedef struct {
	link_type      *head;
	pthread_mutex_t lock;
	pthread_cond_t  cv;
	pthread_mutex_t exit_lock;
	pthread_cond_t  exit_cv;
	int             inited;
	int             count;
	int             wait_count;
} q_type;

/*******************************************************************
 * 		Queue operation functions
 *******************************************************************/

/******************************************************************* 
 * q_init
 *
 * Discreption:
 * 	Initialize the queue before usage.
 *
 * queue	in	The pointer of the queue to initialize
 *
 * Return: 0 on success, Negative value on failure
 *
 * Side Effects: None
 *******************************************************************/
int q_init(q_type *queue);

/******************************************************************* 
 * q_destroy
 *
 * Discretion:
 * 	Destroys the queue. Waits till count of elements in the 
 * 	queue goes to zero.
 *
 * queue	in	The pointer of the queue to destroy
 *
 * Return: None
 *
 * Side Effects: Potentially blocks.
 *******************************************************************/
void q_destroy(q_type *queue);

/******************************************************************* 
 * q_put
 *
 * Discretion:
 * 	Puts element data to the queue. If there are threads
 * 	waiting, then it will unblock one of them. It is 
 * 	undefined on the order of the threads which are unblocked.
 *
 * queue	in	The pointer of the queue to use
 * data		in	The pointr of the data to enqueue.
 *
 * Return: 0 on success, -1 on failure.
 *
 * Side Effects: None
 *******************************************************************/
int q_put(q_type *queue, void *data);

/******************************************************************* 
 * q_get
 *
 * Discretion:
 * 	Removes and returns the first element in the queue. This
 * 	function will block if there are no elements in the queue.
 *
 * queue	in	The pointer of the queue to use
 *
 * Return: pointer of the data on success, 
 * 	   NULL on failure or if the queue is (being) destroyed.
 *
 * Side Effects: If a thread trying to destroy the queue is waiting
 * for the count to go to zero, and the last element in the queue
 * was removed, then this also signals the destroyer thread.
 *******************************************************************/
void *q_get(q_type *q);

/******************************************************************* 
 * q_get_nonblocking
 *
 * Discretion:
 * 	Removes and returns the first element in the queue.
 *
 * queue	in	The pointer of the queue to use
 *
 * Return: pointer of the data on success, 
 * 	   NULL on failure or if there are no elements in the queue.
 *
 * Side Effects: If a thread trying to destroy the queue is waiting
 * for the count to go to zero, and the last element in the queue
 * was removed, then this also signals the destroyer thread.
 *******************************************************************/
void *q_get_nonblocking(q_type *q);

#endif
