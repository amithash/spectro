#ifndef _DECODER_BACKEND_H_
#define _DECODER_BACKEND_H_

#include "decoder_i.h"


struct decoder_buffer_type {
	unsigned int frate;
	float        *buffer;
	unsigned int len;
};

struct decoder_backend_ops {
	int (*open)(void **handle, void *client_handle, char *fname);
	int (*close)(void *handle);
	int (*start)(void *handle);
};

struct decoder_backend_generic_handle
{
	void *client_handle;
};

/* Decoder visible interface */
int decoder_backend_open(struct decoder_handle_struct *handle, char *fname);
int decoder_backend_start(struct decoder_handle_struct *handle);
int decoder_backend_close(struct decoder_handle_struct *handle);


/* Backend visible interface */
int decoder_backend_push(void *handle, float *data, unsigned int len, unsigned int frate);
void decoder_backend_register(struct decoder_backend_ops *ops, char *extension);

#endif
