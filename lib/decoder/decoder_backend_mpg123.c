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

#include "decoder_backend.h"
#include <stdio.h>
#include <stdlib.h>
#include <mpg123.h>
#include <sys/types.h>
#include <string.h>

#define CONVERT_INT_SIGNED(data, in, nsamples, nchannels, halfval) do {	\
	int _i,_j;							\
	for(_i = 0; _i < nsamples; _i++) {				\
		data[_i] = 0;						\
		for(_j = 0; _j < nchannels; _j++) {			\
			data[_i] += (float)(in[_i * nchannels + _j]);	\
		}							\
		data[_i] /= (float)(nchannels * halfval);		\
	}								\
} while(0)

#define CONVERT_INT_UNSIGNED(data, in, nsamples, nchannels, halfval) do {	\
	int _i,_j;								\
	for(_i = 0; _i < nsamples; _i++) {					\
		data[_i] = 0;							\
		for(_j = 0; _j < nchannels; _j++) {				\
			data[_i] += (float)(in[_i * nchannels + _j] - halfval);	\
		}								\
		data[_i] /= (float)(nchannels * halfval);			\
	}									\
} while(0)

struct decoder_mpg123_handle {
	void *client_handle;	/* This should always be first */
	mpg123_handle *mpg123_handle_private;
};

static void decoder_backend_mpg123_decode(void *_handle);
static int decoder_backend_mpg123_close(void *_handle);
static int decoder_backend_mpg123_open(void **_handle, void *client_handle, char *file);

static struct decoder_backend_ops backend_ops = {
	decoder_backend_mpg123_open,
	decoder_backend_mpg123_close,
	decoder_backend_mpg123_decode
};

static int pcm2float_mono_8(float **_data, unsigned int *out_len, unsigned char *_in, size_t in_size, 
			int nchannels, enum mpg123_enc_enum enc)
{
	int8_t *in = (int8_t *)_in;
	float *data;
	unsigned int nsamples = in_size / (nchannels * 1);

	data = malloc(sizeof(float) * nsamples);
	if(!data)
	      return -1;
	switch(enc) {
		case MPG123_ENC_8:
			CONVERT_INT_UNSIGNED(data, in, nsamples, nchannels, (1L<<7));
			break;
		case MPG123_ENC_SIGNED_8:
			CONVERT_INT_SIGNED(data, in, nsamples, nchannels, (1L<<7));
			break;
		default:
			return -1;
	}
	*_data = data;
	*out_len = nsamples;
	return 0;
}
static int pcm2float_mono_16(float **_data, unsigned int *out_len, unsigned char *_in, size_t in_size, 
			int nchannels, enum mpg123_enc_enum enc)
{
	int16_t *in = (int16_t *)_in;
	float *data;
	unsigned int nsamples = in_size / (nchannels * 2);

	data = malloc(sizeof(float) * nsamples);
	if(!data)
	      return -1;
	switch(enc) {
		case MPG123_ENC_16:
		case MPG123_ENC_UNSIGNED_16:
			CONVERT_INT_UNSIGNED(data, in, nsamples, nchannels, (1L<<15));
			break;
		case MPG123_ENC_SIGNED_16:
			CONVERT_INT_SIGNED(data, in, nsamples, nchannels, (1L<<15));
			break;
		default:
			return -1;
	}
	*_data = data;
	*out_len = nsamples;
	return 0;
}
static int pcm2float_mono_32(float **_data, unsigned int *out_len, unsigned char *_in, size_t in_size, 
			int nchannels, enum mpg123_enc_enum enc)
{
	int32_t *in = (int32_t *)_in;
	float *data;
	unsigned int nsamples = in_size / (nchannels * 4);

	data = malloc(sizeof(float) * nsamples);
	if(!data)
	      return -1;
	switch(enc) {
		case MPG123_ENC_UNSIGNED_32:
			CONVERT_INT_UNSIGNED(data, in, nsamples, nchannels, (1L<<31));
			break;
		case MPG123_ENC_SIGNED_16:
			CONVERT_INT_SIGNED(data, in, nsamples, nchannels, (1L<<31));
			break;
		default:
			return -1;
	}
	*_data = data;
	*out_len = nsamples;
	return 0;
}
static int pcm2float_mono_float(float **_data, unsigned int *out_len, unsigned char *_in, size_t in_size, int nchannels)
{
	float *in = (float *)_in;
	float *data;
	int i,j;
	unsigned int nsamples = in_size / (nchannels * sizeof(float));

	data = malloc(sizeof(float) * nsamples);
	if(!data)
	      return -1;
	for(i = 0; i < nsamples; i++) {
		data[i] = 0;
		for(j = 0; j < nchannels; j++) {
			data[i] += in[i*nchannels + j];
		}
		data[i] /= (float)nchannels;
	}
	*_data = data;
	*out_len = nsamples;
	return 0;
}

static int pcm2float_mono_double(float **_data, unsigned int *out_len, unsigned char *_in, size_t in_size, int nchannels)
{
	double *in = (double *)_in;
	float *data;
	int i,j;
	unsigned int nsamples = in_size / (nchannels * sizeof(double));

	data = malloc(sizeof(float) * nsamples);
	if(!data)
	      return -1;

	for(i = 0; i < nsamples; i++) {
		data[i] = 0;
		for(j = 0; j < nchannels; j++) {
			data[i] += (float)in[i*nchannels + j];
		}
		data[i] /= (float)nchannels;
	}
	*_data = data;
	*out_len = nsamples;
	return 0;
}

static int pcm2float_mono(float **_data, unsigned int *out_len, unsigned char *in, size_t in_size, 
			int nchannels, enum mpg123_enc_enum enc)
{
	int ret = 0;
	switch(enc) {
		case MPG123_ENC_8:
		case MPG123_ENC_SIGNED_8:
			ret = pcm2float_mono_8(_data, out_len, in, in_size, nchannels, enc);
			break;
		case MPG123_ENC_16:
		case MPG123_ENC_SIGNED_16:
		case MPG123_ENC_UNSIGNED_16:
			ret = pcm2float_mono_16(_data, out_len, in, in_size, nchannels, enc);
			break;
		case MPG123_ENC_SIGNED_32:
		case MPG123_ENC_UNSIGNED_32:
			ret = pcm2float_mono_32(_data, out_len, in, in_size, nchannels, enc);
			break;
		case MPG123_ENC_FLOAT:
		case MPG123_ENC_FLOAT_32:
			ret = pcm2float_mono_float(_data, out_len, in, in_size, nchannels);
			break;
		case MPG123_ENC_FLOAT_64:
			ret = pcm2float_mono_double(_data, out_len, in, in_size, nchannels);
			break;
		/* AFAIK signed is just a flag to add signedness to other encodings */
		case MPG123_ENC_SIGNED:
		/* Might support adaptive in the future */
		case MPG123_ENC_ULAW_8:
		case MPG123_ENC_ALAW_8:
		/* I cannot support any :-) AFAIK this is used for encoding not decoding */
		case MPG123_ENC_ANY:
		default:
			*_data = NULL;
			*out_len = 0;
			printf("Unsupported encoding: %x\n",enc);
			ret = -1;
	}
	return ret;
}

static int decoder_backend_mpg123_open(void **_handle, void *client_handle, char *file)
{
	struct decoder_mpg123_handle *handle;

	handle = (struct decoder_mpg123_handle *)malloc(sizeof(struct decoder_mpg123_handle));
	*_handle = NULL;
	if(!handle) {
		return -1;
	}
	handle->mpg123_handle_private = mpg123_new(0, 0);
	if(mpg123_open(handle->mpg123_handle_private, file)) {
		mpg123_delete(handle->mpg123_handle_private);
		free(handle);
		return -1;
	}
	handle->client_handle = client_handle;
	*_handle = handle;
	return 0;
}

static int decoder_backend_mpg123_close(void *_handle)
{
	struct decoder_mpg123_handle *handle = (struct decoder_mpg123_handle *)_handle;
	if(!handle)
		return -1;

	mpg123_close(handle->mpg123_handle_private);
	mpg123_delete(handle->mpg123_handle_private);
	free(handle);
	return 0;
}

static void decoder_backend_mpg123_decode(void *_handle)
{
	int rc;
	int in_progress = 1;
	struct decoder_mpg123_handle *handle = (struct decoder_mpg123_handle *)_handle;
	long frate;
	int channels;
	int encoding;
	float *data;
	unsigned int len;
	off_t frame_num;
	size_t size;
	unsigned char *pcm;
	if(!handle)
	      return;
	if(!handle->mpg123_handle_private)
	      return;

	while(in_progress) {
		rc = mpg123_decode_frame(handle->mpg123_handle_private,
					&frame_num, &pcm, &size);
		if(rc != MPG123_OK) {
			if(rc == MPG123_DONE) {
				in_progress = 0;
			} else if(rc == MPG123_NEW_FORMAT) {
				mpg123_getformat(handle->mpg123_handle_private,
							&frate, &channels, &encoding);
			} else {
#ifdef DEBUG
				printf("BACKEND ERROR: %s\n", mpg123_plain_strerror(rc));
#endif
			}
		}
		if(size > 0) {
			data = NULL;
			len = 0;
			if(pcm2float_mono(&data, &len, pcm, size, channels, encoding)) {
				printf("pcm to float conversion failed!\n");
				continue;
			}
			if(!data || !len) {
				printf("Bad conversion\n");
				continue;
			}
			decoder_backend_push(handle, data, len, frate);
		}
	}

	/* Send null packet signalling the client that the stream is done */
	decoder_backend_push(handle, NULL, 0, frate);
}

__attribute__((constructor))
void decoder_backend_mpg123_init(void)
{
	mpg123_init();
	decoder_backend_register(&backend_ops, "mp3");
	decoder_backend_register(&backend_ops, "mp2");
	decoder_backend_register(&backend_ops, "wav");
}

__attribute__((destructor))
void decoder_backend_mpg123_exit(void)
{
	mpg123_exit();
}
