#include "decoder_backend.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <string.h>
#include <vorbis/vorbisfile.h>

struct decoder_example_handle {
	void *client_handle; /* Always the first element */
	pthread_t thread;
	OggVorbis_File file;
	/* Other handles required on a per-file/per-instance basis */
};

/* Required operation functions */
static int decoder_backend_vorbis_start(void *_handle);
static int decoder_backend_vorbis_close(void *_handle);
static int decoder_backend_vorbis_open(void **_handle, void *client_handle, char *file);

/* The function table for this backend */
static struct decoder_backend_ops backend_ops = {
	decoder_backend_vorbis_open,
	decoder_backend_vorbis_close,
	decoder_backend_vorbis_start
};

__attribute__((constructor))
void decoder_backend_vorbis_init(void)
{
	decoder_backend_register(&backend_ops, "ogg");
	decoder_backend_register(&backend_ops, "vob");
}

__attribute__((destructor))
void decoder_backend_vorbis_exit(void)
{

}

static void *decoder_backend_vorbis_thread(void *_handle)
{
	float **buffer;
	unsigned int len;
	unsigned int frate;
	unsigned int channels;
	float *out_buf;
	int bitstream;
	int i,j;
	vorbis_info *info;

	struct decoder_example_handle *handle = 
	    (struct decoder_example_handle *)_handle;

	while(1) {
		len = ov_read_float(&handle->file, &buffer, 2048, &bitstream);
		if(len == 0)
		      break;
		if(len < 0)
		      continue;
		info = ov_info(&handle->file, bitstream);
		frate = info->rate;
		channels = info->channels;

		out_buf = (float *)malloc(sizeof(float) * len);
		if(!out_buf)
		      break;
		for(i = 0; i < len; i++) {
			out_buf[i] = 0;
			for(j = 0; j < channels; j++)
			      out_buf[i] += buffer[i][j];
		}

		decoder_backend_push(handle, out_buf, len, frate);
	}

	decoder_backend_push(handle, NULL, 0, 0);

	pthread_exit(NULL);
}

static int decoder_backend_vorbis_open(void **_handle, void *client_handle, char *file)
{
	struct decoder_example_handle *handle;

	*_handle = NULL;
	handle = (struct decoder_example_handle *)
	    malloc(sizeof(struct decoder_example_handle));

	if(!handle)
	      return -1;
	handle->client_handle = client_handle;

	if(ov_fopen(file, &handle->file)){
		free(handle);
		return -1;
	}

	*_handle = handle;
	return 0;
}

static int decoder_backend_vorbis_start(void *_handle)
{
	struct decoder_example_handle *handle = 
	    (struct decoder_example_handle *)_handle;
	int rc;

	if(!handle)
	      return -1;

	rc = pthread_create(&handle->thread, 
			    0,               
			    decoder_backend_vorbis_thread, 
			    handle 
			    );
	if(!rc) {
		return -1;
	}

	return 0;
}

static int decoder_backend_vorbis_close(void *_handle)
{
	struct decoder_example_handle *handle = 
	    (struct decoder_example_handle *)_handle;
	void *pars;

	if(!handle)
	      return -1;

	pthread_join(handle->thread, &pars);

	ov_clear(&handle->file);

	free(handle);

	return 0;
}

