#ifndef SPECTGEN_SESSION_H_
#define SPECTGEN_SESSION_H_

#include <pthread.h>
#include <fftw3.h>
#include "decoder.h"

typedef void (*user_session_cb_t)(void *);

typedef struct spectgen_session_struct {
	unsigned int                   window_size;
	unsigned int                   numfreqs;
	pthread_mutex_t                lock;
	pthread_cond_t                 cond;
	pthread_t                      thread;
	decoder_handle                 d_handle;
	volatile void                  *user_handle;
	volatile user_session_cb_t     user_cb;
	int                            working;
	pthread_mutex_t                wait_lock;
	pthread_cond_t                 wait_cond;
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
