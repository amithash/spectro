/*******************************************************************************
    This file is part of spectro
    Copyright (C) 2011  Amithash Prasad <amithash@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "decoder_backend.h"

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
