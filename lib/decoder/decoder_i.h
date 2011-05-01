#ifndef _DECODER_I_H_
#define _DECODER_I_H_

#include <pthread.h>
#include "queue.h"
#include "decoder.h"

typedef struct {
	int signal;
	pthread_mutex_t mutex;
	pthread_cond_t  cond;
} signal_type;

#define SIGNAL_INIT(sig) do {						\
	(sig)->signal = 0;						\
	pthread_mutex_init(&((sig)->mutex), NULL);			\
	pthread_cond_init(&((sig)->cond), NULL);			\
} while(0)

#define SIGNAL_DEINIT(sig) do {						\
	(sig)->signal = 0;						\
	pthread_mutex_destroy(&((sig)->mutex));				\
	pthread_cond_destroy(&((sig)->cond));				\
} while(0)

#define SIGNAL_SET(sig) do {						\
	pthread_mutex_lock(&((sig)->mutex));				\
	(sig)->signal = 1;						\
	pthread_cond_broadcast(&((sig)->cond));				\
	pthread_mutex_unlock(&((sig)->mutex));				\
} while(0)

#define SIGNAL_WAIT(sig) do {						\
	pthread_mutex_lock(&((sig)->mutex));				\
	while((sig)->signal == 0) {					\
		pthread_cond_wait(&((sig)->cond), &((sig)->mutex));	\
	}								\
	(sig)->signal = 0;						\
	pthread_mutex_unlock(&((sig)->mutex));				\
} while(0)

struct decoder_handle_struct
{
	volatile void *backend_handle; /* Populated by backend */
	volatile void *backend_info;   /* Populated by backend AL */
	q_type *queue;
	float *acc_data;
	unsigned int acc_len;
	long last_frate;
	
	pthread_t thread;
	int active;
	signal_type     start_signal;
	signal_type     finished_signal;
};

#endif
