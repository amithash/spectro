#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "queue.h"


int q_init(q_type *q)
{
	if(!q)
	      return -1;
	q->head = NULL;
	q->tail = NULL;
	q->inited = 1;
	q->wait_count = 0;
	q->count = 0;
	pthread_mutex_init(&q->lock, NULL);
	pthread_cond_init(&q->cv, NULL);
	pthread_mutex_init(&q->exit_lock, NULL);
	pthread_cond_init(&q->exit_cv, NULL);
	
	return 0;
}

void q_destroy(q_type *q)
{
	if(!q->inited)
	      return;
	pthread_mutex_lock(&q->lock);
	q->inited = 0;

	/* Wait till the q is flushed */
	while(q->count > 0) {
		pthread_cond_wait(&q->exit_cv, &q->exit_lock);
	}
	q->head = NULL;
	q->tail = NULL;
	pthread_mutex_unlock(&q->lock);

	pthread_mutex_destroy(&q->lock);
	pthread_cond_destroy(&q->cv);
	pthread_mutex_destroy(&q->exit_lock);
	pthread_cond_destroy(&q->exit_cv);
}

int q_put(q_type *q, void *data)
{
	link_type *link;

	if(q == NULL)
	      return -1;

	if(q->inited == 0)
	      return -1;

	link = (link_type *)malloc(sizeof(link_type));
	if(!link)
	      return -1;

	link->data = data;
	link->next = NULL;
	pthread_mutex_lock(&q->lock);
	if(!q->head) {
		q->tail = q->head = link;
	} else {
		q->tail->next = link;
		q->tail = link;
	}
	q->count++;

	if(q->wait_count > 0) {
		pthread_cond_signal(&q->cv);
	}
	pthread_mutex_unlock(&q->lock);
	return 0;
}

static void *_q_get(q_type *q, int blocking)
{
	link_type *link;
	void *data;
	if(!q)
	      return NULL;

	/* if queue is not initialized & count is 0 */
	if(!q->inited && !q->count)
	      return NULL;

	pthread_mutex_lock(&q->lock);
	if(blocking == 0 && q->count == 0) {
		pthread_mutex_unlock(&q->lock);
		return NULL;
	}

	q->wait_count++;
	while(blocking == 1 && q->count == 0)
	      pthread_cond_wait(&q->cv, &q->lock);
	q->wait_count--;

	link = q->head;
	q->head = q->head->next;
	if(!q->head)
	      q->tail = NULL;

	q->count--;
	data = link->data;
	free(link);
	pthread_mutex_unlock(&q->lock);

	if(q->inited == 0 && q->count == 0) {
		/* Someone is waiting to destroy the queue */
		pthread_cond_signal(&q->exit_cv);
		pthread_mutex_unlock(&q->exit_lock);
	}
	return data;
}

void *q_get(q_type *q)
{
	return _q_get(q, 1);
}

void *q_try_get(q_type *q)
{
	return _q_get(q, 0);
}

