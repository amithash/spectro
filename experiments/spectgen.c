#include <stdio.h>
#include <mpg123.h>
#include <sys/types.h>
#include <string.h>
#include <fftw3.h>
#include <math.h>

typedef int8_t b1_t;
typedef int16_t b2_t;
typedef int32_t b4_t;
typedef int64_t b8_t;

#define UINT8_CENTER  (1 << 7)
#define UINT16_CENTER (1 << 16)
#define UINT32_CENTER (1 << 31)

#define CONVERT(data, in, nsamples, nchannels) do {			\
	int _i,_j;							\
	for(_i = 0; _i < nsamples; _i++) {				\
		data[_i] = 0;						\
		for(_j = 0; _j < nchannels; _j++) {			\
			int _val = (int)in[_i * nchannels + _j];	\
			data[_i] += _val;				\
		}							\
	}								\
} while(0)

#define MAX_BUFFER_SIZE (1024 * 1024 * 512)
#define WINDOW_SIZE 1024
#define STEP_SIZE 512

int do_band(fftwf_complex *buf)
{

}

int do_fft(float *in, unsigned int len)
{
	fftwf_complex *tmp_out;
	float *tmp_in;
	float *buf = in;
	int i;
	fftwf_plan plan;

	tmp_out = (fftwf_complex *)malloc(sizeof(fftwf_complex) * WINDOW_SIZE / 2 + 1);
	if(!tmp_out) {
		goto tmp_out_failed;
	}
	tmp_in = (float *)malloc(sizeof(float) * WINDOW_SIZE);
	if(!tmp_in)
	      goto tmp_in_failed;

	plan = fftwf_plan_dft_r2c_1d(WINDOW_SIZE, tmp_in, tmp_out, FFTW_MEASURE);
	for(i = 0; i < len; i+=STEP_SIZE) {
		if(i + WINDOW_SIZE > len)
		      break;
		memcpy(tmp_in, buf, WINDOW_SIZE * sizeof(float));
		fftwf_execute(plan);
		buf += WINDOW_SIZE;
		do_band(tmp_out);
	}
	fftwf_destroy_plan(plan);
	free(tmp_in);
tmp_in_failed:
	free(tmp_out);
tmp_out_failed:
	return 0;
}

void char2float_2(float *data, unsigned char *_in, int nchannels, int nsamples)
{
	b2_t *in = (b2_t *)_in;
	printf("Bytes per channel = 2\n"); fflush(stdout);
	CONVERT(data, in, nsamples, nchannels);
}
void char2float_1(float *data, unsigned char *_in, int nchannels, int nsamples)
{
	b1_t *in = (b1_t *)_in;
	printf("Bytes per channel = 1\n"); fflush(stdout);
	CONVERT(data, in, nsamples, nchannels);
}
void char2float_4(float *data, unsigned char *_in, int nchannels, int nsamples)
{
	b4_t *in = (b4_t *)_in;
	printf("Bytes per channel = 4\n"); fflush(stdout);
	CONVERT(data, in, nsamples, nchannels);
}

void char2float(float *data, unsigned char *in, int nchannels, int nsamples, int size_per_sample)
{
	int size_per_channel = size_per_sample / nchannels;
	switch(size_per_channel) {
		case 1:
			char2float_1(data, in, nchannels, nsamples);
			break;
		case 2:
			char2float_2(data, in, nchannels, nsamples);
			break;
		case 4:
			char2float_4(data, in, nchannels, nsamples);
			break;
		default:
			printf("Unsupported\n");
			break;
	}
}

int main(int argc, char *argv[])
{
	mpg123_handle *handle;
	unsigned char *audio;
	int mc;
	size_t size;
	unsigned int total_len = 0;
	off_t frame_num;
	unsigned int total_size = 0;
	unsigned int samples;
	long frate;
	int channels;
	int encoding;
	int bperchannel;
	float *data_f;
	unsigned char *data;
	int i;
	char *buffer;


	if(argc < 2) {
		printf("USAGE: %s <MP3>\n",argv[0]);
		exit(-1);
	}

	buffer = (char *)malloc(sizeof(char) * MAX_BUFFER_SIZE);

	mpg123_init();
	handle = mpg123_new(0,0);
	if(mpg123_open(handle, argv[1]) != MPG123_OK) {
		printf("Could not open %s to decode\n", argv[1]);
		exit(-1);
	}
	data = buffer;
	while(1) {
		mc = mpg123_decode_frame(handle, &frame_num, &audio, &size);
		if(size > 0) {
			total_size += size;
			if(total_size > MAX_BUFFER_SIZE) {
				printf("Ran out of memory!\n");
				exit(-1);
			}
			memcpy(data, audio, size);
			data += size / sizeof(char);
		}

		if(mc != MPG123_OK) {
			if(mc == MPG123_ERR || mc == MPG123_NO_SPACE ) {
				printf("Error encountered in decoding...\n");
				break;
			}
			if(mc == MPG123_DONE) {
				printf("Decoding done!\n");
				break;
			}
			if(mc == MPG123_NEW_FORMAT) {
				mpg123_getformat(handle, &frate, &channels, &encoding);
				printf("Frame Rate = %d\n", frate);
				printf("Num Channels = %d\n", channels);
			}
		}
	}
	printf("Copied over %lu bytes to buffer\n", (unsigned long)data - (unsigned long)buffer);
	samples = mpg123_length(handle);
	mpg123_close(handle);
	mpg123_delete(handle);
	mpg123_exit();

	data_f = (float *) malloc(samples * sizeof(float));
	if(!data_f) {
		printf("Malloc failed\n");
		exit(-1);
	}
	printf("Read %u frames\n", (unsigned int)frame_num);
	printf("Read %u bytes\n", (unsigned int)total_size);
	printf("Total samples = %d\n", samples);
	printf("Bytes per sample = %d\n", total_size / samples);
	fflush(stdout);
	char2float(data_f, buffer, channels, samples, total_size / samples);
	free(buffer);
	do_fft(data_f, samples);

#if 0
	for(i = 0; i < samples; i++) {
		printf("%.3f\n", data_f[i]);
	}
#endif

	return 0;
}

