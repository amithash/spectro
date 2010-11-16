#include "hist_dist.h"

void cent_clear(void *h)
{
	int i,j;
	hist_t *hist = (hist_t *)h;
	hist->fname[0] = '\0';
	for(i = 0; i < NBANDS; i++) {
		for(j = 0; j < SPECT_HIST_LEN; j++) {
			hist->spect_hist[i][j] = 0;
		}
	}
}

void cent_accum(void *_out, void *_in)
{
	int i,j;
	hist_t *out = (hist_t *)_out;
	hist_t *in  = (hist_t *)_in;
	for(i = 0; i < NBANDS; i++) {
		for(j = 0; j < SPECT_HIST_LEN; j++) {
			out->spect_hist[i][j] += sqrt(in->spect_hist[i][j]);
		}
	}
}

void cent_final(void *_out, void *_in, unsigned int len)
{
	int i,j;
	hist_t *out = (hist_t *)_out;
	hist_t *in  = (hist_t *)_in;

	for(i = 0; i < NBANDS; i++) {
		for(j = 0; j < SPECT_HIST_LEN; j++) {
			out->spect_hist[i][j] = pow(in->spect_hist[i][j] / len, 2);
		}
	}
}

void cent_copy(void *_out, void *_in) 
{
	hist_t *out = (hist_t *)_out;
	hist_t *in  = (hist_t *)_in;
	memcpy(out, in, sizeof(hist_t));
}

void *hist_ind(void *_data, int i)
{
	hist_t *data = (hist_t *)_data;
	return (void *)&data[i];
}

void *hist_calloc(int len)
{
	hist_t *ret;

	ret = (hist_t *)calloc(len, sizeof(hist_t));
	return (void *)ret;
}

float hist_dist(void *a, void *b)
{
	return hist_distance((hist_t *)a, (hist_t *)b);
}

