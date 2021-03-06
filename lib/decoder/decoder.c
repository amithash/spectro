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
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include "decoder_i.h"
#include "decoder_backend.h"

/*********************************************************
 * 		Private Functions
 ********************************************************/

static void *decoder_thread(void *data)
{
	struct decoder_handle_struct *handle = (struct decoder_handle_struct *)data;
	while(handle->active) {
		SIGNAL_WAIT(&handle->start_signal);
		if(handle->active == 0)
		      break;
		decoder_backend_decode(handle);
		SIGNAL_SET(&handle->finished_signal);
	}
	pthread_exit(NULL);
}

/*********************************************************
 * 		Public Functions
 ********************************************************/
/* decoder_init
 *
 * Initialize a pipeline to handle decoding.
 * If multiple disjoint threads want to handle decoding
 * of separate audio files, then this must be called on
 * each of them.
 *
 * handle	out	decoder handle to be used with each
 * 			transaction with the decoder.
 *
 * Return: 0 on success, negative error code on failure.
 */
int decoder_init(decoder_handle *_handle)
{
	struct decoder_handle_struct *handle;

	*_handle = NULL;
	handle = (struct decoder_handle_struct *)
	    		malloc(sizeof(struct decoder_handle_struct));
	if(!handle)
	      goto handle_malloc_failed;
	handle->queue = (q_type *)malloc(sizeof(q_type));
	if(!handle->queue)
	      goto queue_malloc_failed;

	if(q_init(handle->queue))
	      goto queue_init_failed;
	handle->active = 1;
	SIGNAL_INIT(&handle->start_signal);
	SIGNAL_INIT(&handle->finished_signal);

	if(pthread_create(&handle->thread, 0, decoder_thread, handle))
	      goto thread_creation_failed;

	*_handle = handle;
	return 0;
thread_creation_failed:
	SIGNAL_DEINIT(&handle->start_signal);
	SIGNAL_DEINIT(&handle->finished_signal);
queue_init_failed:
	free(handle->queue);
queue_malloc_failed:
	free(handle);
handle_malloc_failed:
	return -1;
}

/* decoder_open
 *
 * Open an audio file and prepare for processing.
 *
 * handle	in	decoder handle from decoder_init
 *
 * Return: 0 on success, negative error code on failure.
 */
int decoder_open(decoder_handle _handle, char *fname)
{
	struct decoder_handle_struct *handle = (struct decoder_handle_struct *)_handle;
	if(!handle)
	      return -1;
	if(!fname)
	      return -1;
	handle->backend_handle = NULL;
	return decoder_backend_open(handle, fname);
}

/* decoder_start
 *
 * Start decoding of the file opened in decoder_open.
 * Note that the user needs to use decoder_data_pull to
 * get the decoded sample data.
 *
 * handle	in	decoder handle from decoder_init
 *
 * Return: 0 on success, negative error code on failure.
 */
int decoder_start(decoder_handle _handle)
{
	struct decoder_handle_struct *handle = (struct decoder_handle_struct *)_handle;
	if(!handle && !handle->backend_handle) {
		return -1;
	}
	SIGNAL_SET(&handle->start_signal);
	return 0;
}

int decoder_close(decoder_handle _handle)
{
	struct decoder_handle_struct *handle = (struct decoder_handle_struct *)_handle;
	int rc = 0;
	if(!handle && !handle->backend_handle) {
		return -1;
	}
	SIGNAL_WAIT(&handle->finished_signal);
	rc = decoder_backend_close(handle);
	handle->backend_handle = NULL;
	return rc;
}

void decoder_exit(decoder_handle _handle)
{
	struct decoder_handle_struct *handle = (struct decoder_handle_struct *)_handle;
	void *par;
	if(!handle)
		return;
	handle->active = 0;
	/* So that the active flag is tested */
	SIGNAL_SET(&handle->start_signal);
	pthread_join(handle->thread, &par);

	if(handle->backend_handle)
		decoder_backend_close(handle);
	handle->backend_handle = NULL;
	q_destroy(handle->queue);
	free(handle->queue);
	SIGNAL_DEINIT(&handle->start_signal);
	SIGNAL_DEINIT(&handle->finished_signal);
	free(handle);
}

void decoder_data_pull(decoder_handle _handle, float **_buffer, 
			unsigned int *_len, unsigned int *_frate)
{
	struct decoder_handle_struct *handle = (struct decoder_handle_struct *)_handle;
	struct decoder_buffer_type *buf;
	buf = q_get(handle->queue);
	if(!buf) {
		*_buffer = NULL;
		*_len    = 0;
		*_frate = 0;
		printf("Error in getting input\n");
	} else {
		*_buffer = buf->buffer;
		*_len    = buf->len;
		*_frate  = buf->frate;
	}
	free(buf);
}
void decoder_supported_extensions(char **extensions, unsigned int *out_len)
{
	decoder_backend_supported_extensions(extensions, out_len);
}
