#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "decoder_backend.h"

struct decoder_backend_struct
{
	struct decoder_backend_ops *ops;
	char   ext[5];
	struct decoder_backend_struct *next;
};

static struct decoder_backend_struct *head = NULL;
static pthread_mutex_t head_mutex = PTHREAD_MUTEX_INITIALIZER;

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

int decoder_backend_push(void *_handle, float *data, unsigned int len, unsigned int frate)
{
	struct decoder_handle_struct *handle;
	struct decoder_backend_generic_handle *bhandle = 
	    (struct decoder_backend_generic_handle *)_handle;
	
	struct decoder_buffer_type *buf;

	if(!bhandle || !bhandle->client_handle) {
		return -1;
	}
	handle = (struct decoder_handle_struct *)bhandle->client_handle;

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

int decoder_backend_open(struct decoder_handle_struct *handle, char *fname)
{
	struct decoder_backend_struct *bhandle;
	char ext[5];
	get_file_extension(ext, fname);

	bhandle = lookup(ext);
	if(!bhandle || !bhandle->ops ||!bhandle->ops->open) {
		return -1;
	}
	handle->backend_info = bhandle;
	return bhandle->ops->open(&handle->backend_handle, handle, fname);
}

#define PRINT(fmt) printf(fmt "\n"); fflush(stdout)
int decoder_backend_start(struct decoder_handle_struct *handle)
{
	struct decoder_backend_struct *info;
	if(!handle)
	      return -1;
	info = (struct decoder_backend_struct *)handle->backend_info;
	if(!info || !info->ops || !info->ops->start) {
		return -1;
	}
	return info->ops->start(handle->backend_handle);
}

int decoder_backend_close(struct decoder_handle_struct *handle)
{
	struct decoder_backend_struct *info;
	if(!handle)
	      return -1;
	info = (struct decoder_backend_struct *)handle->backend_info;
	if(!info || !info->ops || !info->ops->close)
	      return -1;
	info->ops->close(handle->backend_handle);
	handle->backend_handle = NULL;
	handle->backend_info = NULL;
	return 0;
}

