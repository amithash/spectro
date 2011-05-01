#ifndef SPECTGEN_SESSION_H_
#define SPECTGEN_SESSION_H_

#include <pthread.h>
#include <fftw3.h>
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


typedef void (*user_session_cb_t)(void *);

typedef struct spectgen_session_struct {
	unsigned int                   window_size;
	unsigned int                   numfreqs;
	pthread_mutex_t                lock;
	pthread_t                      thread;
	decoder_handle                 d_handle;
	volatile void                  *user_handle;
	volatile user_session_cb_t     user_cb;
	signal_type                    start_signal;
	signal_type                    finished_signal;
	volatile int                   active;
	int                            busy;
	float                          *fft_in;
	fftwf_complex                  *fft_out;
	fftwf_plan                     plan;
	struct spectgen_session_struct *next;
} spectgen_session_t;


spectgen_session_t *spectgen_session_get(unsigned int window_size,
					 void *user_handle,
					 user_session_cb_t user_cb
			);
void spectgen_session_start(spectgen_session_t *session);
void spectgen_session_put(spectgen_session_t *session);


#endif
