#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "decoder_backend.h"

#define DECODER_MIN_PUSH_LEN (2048 * 8)

/* If enabled, the decder will accumulate the above vals before pushing */

struct decoder_backend_struct
{
	struct decoder_backend_ops *ops;
	char   ext[5];
	struct decoder_backend_struct *next;
};

static struct decoder_backend_struct *head = NULL;
static pthread_mutex_t head_mutex = PTHREAD_MUTEX_INITIALIZER;

void decoder_backend_supported_extensions(char **extensions, unsigned int *out_len)
{
	int i = 0;
	int max_len = *out_len;
	struct decoder_backend_struct *node;

	pthread_mutex_lock(&head_mutex);
	node = head;
	for(i = 0; i < max_len; i++) {
		if(!node)
		      break;
		strcpy(extensions[i], node->ext);
		node = node->next;
	}
	pthread_mutex_unlock(&head_mutex);
	*out_len = i;
}

static void get_file_extension(char *out, char *in)
{
	int len = strlen(in);
	int i;
	int ind = 0;
	for(i = len; i >= 0; i--) {
		if(in[i] == '.') {
			ind = i;
			break;
		}
	}
	if(!ind)
	      return;
	ind++;
	for(i = 0; i < 4; i++) {
		char a = in[ind+i];
		if(a >= 'A' && a <= 'Z') {
			a = (a - 'A') + 'a';
		}
		out[i] = a;
	}
}

static struct decoder_backend_struct *lookup(char *extension)
{
	struct decoder_backend_struct *i;
	pthread_mutex_lock(&head_mutex);
	i = head;
	while(i) {
		if(strcmp(i->ext, extension) == 0)
		      break;
		i = i->next;
	}
	pthread_mutex_unlock(&head_mutex);
	return i;
}

void decoder_backend_register(struct decoder_backend_ops *ops, char *extension)
{
	struct decoder_backend_struct *link;

	link = (struct decoder_backend_struct *)malloc(sizeof(struct decoder_backend_struct));
	strcpy(link->ext, extension);
	link->ops = ops;
	pthread_mutex_lock(&head_mutex);
	link->next = head;
	head = link;
	pthread_mutex_unlock(&head_mutex);
}


void decoder_backend_deregister(struct decoder_backend_ops *ops)
{
	struct decoder_backend_struct *link;
	struct decoder_backend_struct *prev = NULL;
	pthread_mutex_lock(&head_mutex);
	link = head;
	while(link) {
		if(link->ops == ops) {
			struct decoder_backend_struct *to_free;
			if(prev) {
				head = link->next;
			} else {
				prev->next = link->next;
			}
			to_free = link;
			link = link->next;
			free(to_free);
			continue;
		}
		prev = link;
		link = link->next;
	}

	pthread_mutex_unlock(&head_mutex);
}

static int decoder_backend_push_internal(struct decoder_handle_struct *handle, 
			float *data, unsigned int len, long frate)
{
	struct decoder_buffer_type *buf;

	buf = (struct decoder_buffer_type *)malloc(sizeof(struct decoder_buffer_type));
	if((!buf))
	      return -1;
	buf->buffer = data;
	buf->len = len;
	buf->frate = frate;
	if(q_put(handle->queue, buf)) {
		free(buf);
		return -1;
	}
	return 0;
}

static int accumulate_and_push(struct decoder_handle_struct *handle,
			float *data, unsigned int len, long frate)
{
#if DECODER_MIN_PUSH_LEN > 0
	if(handle->last_frate != frate && handle->acc_len > 0) {
		decoder_backend_push_internal(handle, handle->acc_data, handle->acc_len, handle->last_frate);
		handle->last_frate = frate;
		handle->acc_data = NULL;
		handle->acc_len = 0;
		return accumulate_and_push(handle, data, len, frate);
	}

	if(handle->acc_data == NULL && 
		!(handle->acc_data = (float *)malloc(sizeof(float) * 
				DECODER_MIN_PUSH_LEN))) {
		printf("Warning: MEMORY ERROR: ALLOC Failure\n");
		return -1;
	}

	handle->last_frate = frate;

	if(data == NULL) {
		if(handle->acc_len > 0) {
			if(handle->last_frate > 0) {
				decoder_backend_push_internal(handle, handle->acc_data, handle->acc_len, frate);
			} else {
				/* Cleanup, If we cannot provide a frate, something is definately
				 * wrong, so let the user not get anything */
				free(handle->acc_data);
			}
			handle->acc_data = NULL;
			handle->acc_len = 0;
		}
		decoder_backend_push_internal(handle, NULL, 0, 0);
		return 0;
	} 

	if((handle->acc_len + len) >= DECODER_MIN_PUSH_LEN) {
		handle->acc_data = (float *)realloc(handle->acc_data,
					sizeof(float) * (handle->acc_len + len));
		memcpy(&handle->acc_data[handle->acc_len], data, len * sizeof(float));
		free(data);
		/* Push only if last_frate > 0 */
		if(handle->last_frate > 0) {
			decoder_backend_push_internal(handle, handle->acc_data, handle->acc_len + len, frate);
			handle->acc_data = NULL;
			handle->acc_len = 0;
		}
	} else {
		memcpy(&handle->acc_data[handle->acc_len], data, len * sizeof(float));
		free(data);
		handle->acc_len += len;
	}
#else
	if(decoder_backend_push_internal(handle, data, len, frate))
	      return -1;
#endif
	return 0;
}


int decoder_backend_push(void *_handle, float *data, unsigned int len, unsigned int frate)
{
	struct decoder_handle_struct *handle;
	struct decoder_backend_generic_handle *bhandle = 
	    (struct decoder_backend_generic_handle *)_handle;
	
	if(!bhandle || !bhandle->client_handle) {
		return -1;
	}
	handle = (struct decoder_handle_struct *)bhandle->client_handle;

	return accumulate_and_push(handle, data, len, frate);
}

int decoder_backend_open(struct decoder_handle_struct *handle, char *fname)
{
	struct decoder_backend_struct *bhandle;
	char ext[5];
	get_file_extension(ext, fname);

	handle->acc_data = NULL;
	handle->acc_len = 0;
	handle->last_frate = 0;

	bhandle = lookup(ext);
	if(!bhandle || !bhandle->ops ||!bhandle->ops->open) {
		return -1;
	}
	
	handle->backend_info = bhandle;
	return bhandle->ops->open((void **)&handle->backend_handle, handle, fname);
}

void decoder_backend_decode(struct decoder_handle_struct *handle)
{
	struct decoder_backend_struct *info;
	if(!handle)
	      return;
	info = (struct decoder_backend_struct *)handle->backend_info;
	if(!info || !info->ops || !info->ops->decode)
	      return;
	info->ops->decode((void *)handle->backend_handle);
}

int decoder_backend_close(struct decoder_handle_struct *handle)
{
	struct decoder_backend_struct *info;
	if(!handle)
	      return -1;
	info = (struct decoder_backend_struct *)handle->backend_info;
	if(!info || !info->ops || !info->ops->close)
	      return -1;
	/* TODO: Wait for the thread to signal that its work is done */
	info->ops->close((void *)handle->backend_handle);
	handle->backend_handle = NULL;
	handle->backend_info = NULL;
	return 0;
}

