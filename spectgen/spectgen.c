#include <stdio.h>
#include <mpg123.h>
#include <sys/types.h>
#include <string.h>
#include <fftw3.h>
#include <math.h>
#include <unistd.h>

typedef int8_t b1_t;
typedef int16_t b2_t;
typedef int32_t b4_t;
typedef int64_t b8_t;

#define MAX_LEN(bytes) ((float)(1L << (bytes * 8)))

#define CONVERT(data, in, nsamples, nchannels, bytes) do {		\
	int _i,_j;							\
	for(_i = 0; _i < nsamples; _i++) {				\
		data[_i] = 0;						\
		for(_j = 0; _j < nchannels; _j++) {			\
			data[_i] += (float)((in)[_i * nchannels + _j]);	\
		}							\
		data[_i] /= (float)(nchannels * MAX_LEN(bytes));	\
	}								\
} while(0)

#define CHAR2FLOAT(data, in, nsamples, nchannels, bytes) \
    CONVERT(data, ((b##bytes##_t *)in), nsamples, nchannels, bytes)

#define MAX_BUFFER_SIZE (1024 * 1024 * 512)
#define WINDOW_SIZE 1024
#define STEP_SIZE 512
#define NUMFREQS ((WINDOW_SIZE / 2) + 1)

#define SPECTRUM_BAND_FREQ(band, size, rate) \
      (unsigned int)(((float)(band))*((float)(rate))/((float)(size)))
#define BAND_FREQ(i, sr) SPECTRUM_BAND_FREQ(i, WINDOW_SIZE, sr)


unsigned int *bark_band_table;
static const unsigned int bark_bands[24] 
  = { 100,  200,  300,  400,  510,  630,  770,   920, 
      1080, 1270, 1480, 1720, 2000, 2320, 2700,  3150, 
      3700, 4400, 5300, 6400, 7700, 9500, 12000, 15500 };


int do_band(float *buf, int fn)
{
	int i;
	float band[24] = {0};
	for(i = 0; i < 24; i++)
	      band[i] = 0;

	for(i = 1; i < NUMFREQS; i++) {
		float real = buf[2 * i];
		float imag = buf[2 * i + 1];
		band[bark_band_table[i]] += ((real * real) + (imag * imag)) / (float)WINDOW_SIZE;
	}
	for(i = 0; i < 24; i++) {
		band[i] = sqrt(band[i]);
	}
	if(write(fn, band, 24 * sizeof(float)) != sizeof(float) * 24) {
		printf("Write failed\n");
	}
	return 0;
}

void setup_bark_band_table(unsigned int sampling_rate)
{
	int i;
	unsigned int barkband = 0;
	bark_band_table = (unsigned int *)malloc(sizeof(unsigned int) * NUMFREQS);
	for(i = 0; i < NUMFREQS; i++)
	      bark_band_table[i] = 0;
	for(i = 0; i < NUMFREQS; i++) {
		if(barkband < 23 &&
			BAND_FREQ(i, sampling_rate) >= bark_bands[barkband])
		      barkband++;
		bark_band_table[i] = barkband;
	}
}

int do_fft(float *in, unsigned int len, unsigned int sampling_rate, int fd)
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

	setup_bark_band_table(sampling_rate);

	plan = fftwf_plan_dft_r2c_1d(WINDOW_SIZE, tmp_in, tmp_out, FFTW_MEASURE);

	for(i = 0; i < len; i+=STEP_SIZE) {
		if(i + WINDOW_SIZE > len)
		      break;
		memcpy(tmp_in, buf, WINDOW_SIZE * sizeof(float));
		fftwf_execute(plan);
		buf += STEP_SIZE;
		do_band((float *)tmp_out, fd);
	}
	fftwf_destroy_plan(plan);
	free(tmp_in);
	free(bark_band_table);
tmp_in_failed:
	free(tmp_out);
tmp_out_failed:
	return 0;
}

void char2float(float *data, char *in, int nchannels, int nsamples, int size_per_sample)
{
	int size_per_channel = size_per_sample / nchannels;
	switch(size_per_channel) {
		case 1:
			CHAR2FLOAT(data, in, nsamples, nchannels, 1);
			break;
		case 2:
			CHAR2FLOAT(data, in, nsamples, nchannels, 2);
			break;
		case 4:
			CHAR2FLOAT(data, in, nsamples, nchannels, 4);
			break;
		default:
			printf("Unsupported bytes per channel %d\n", size_per_channel);
			break;
	}
}

int main(int argc, char *argv[])
{
	mpg123_handle *handle;
	unsigned char *audio;
	int mc;
	size_t size;
	off_t frame_num;
	unsigned int total_size = 0;
	unsigned int samples;
	long frate;
	int channels;
	int encoding;
	float *data_f;
	char *data;
	char *buffer;
	char *inf;
	char *outf;
	FILE *f;

	if(argc < 4) {
		printf("USAGE: %s <MP3>\n",argv[0]);
		exit(-1);
	}
	inf = argv[1];
	outf = argv[3];

	buffer = (char *)malloc(sizeof(char) * MAX_BUFFER_SIZE);

	mpg123_init();
	handle = mpg123_new(0,0);
	if(mpg123_open(handle, inf) != MPG123_OK) {
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
				break;
			}
			if(mc == MPG123_NEW_FORMAT) {
				mpg123_getformat(handle, &frate, &channels, &encoding);
			}
		}
	}
	samples = mpg123_length(handle);
	mpg123_close(handle);
	mpg123_delete(handle);
	mpg123_exit();

	data_f = (float *) malloc(samples * sizeof(float));
	if(!data_f) {
		printf("Malloc failed\n");
		exit(-1);
	}
	char2float(data_f, buffer, channels, samples, total_size / samples);
	free(buffer);
	f = fopen(outf, "w");
	if(!f) {
		printf("Could not create %s\n", outf);
		exit(-1);
	}
	do_fft(data_f, samples, frate, fileno(f));
	fclose(f);

	return 0;
}

