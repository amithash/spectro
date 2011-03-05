#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "decoder_i.h"
#include "decoder_backend.h"

/*********************************************************
 * 		Private Functions
 ********************************************************/

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

	*_handle = handle;
	return 0;

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
	return decoder_backend_start(handle);
}

int decoder_close(decoder_handle _handle)
{
	struct decoder_handle_struct *handle = (struct decoder_handle_struct *)_handle;
	int rc = 0;
	if(!handle && !handle->backend_handle) {
		return -1;
	}
	rc = decoder_backend_close(handle);
	handle->backend_handle = NULL;
	return rc;
}

void decoder_exit(decoder_handle _handle)
{
	struct decoder_handle_struct *handle = (struct decoder_handle_struct *)_handle;
	if(!handle)
		return;
	if(handle->backend_handle)
		decoder_backend_close(handle);
	handle->backend_handle = NULL;
	q_destroy(handle->queue);
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
