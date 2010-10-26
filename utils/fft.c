#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <fftw3.h>

typedef struct
{
	fftw_complex *in, *out;
	fftw_plan    forward;
	fftw_plan    backward;
	int          size;
} fft_plan_t;


void * init_fft(int size)
{
	fft_plan_t *p = (fft_plan_t *)malloc(sizeof(fft_plan_t));

	p->in       = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * size);
	p->out      = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * size);
	p->forward  = fftw_plan_dft_1d(size, p->in, p->out, FFTW_FORWARD, FFTW_ESTIMATE);
	p->backward = fftw_plan_dft_1d(size, p->in, p->out, FFTW_BACKWARD, FFTW_ESTIMATE);
	p->size  = size;

	return (void *)p;
}

void destroy_fft(void *handle)
{
	fft_plan_t *p;
	if(!handle)
	      return;
	p = (fft_plan_t *)handle;

	fftw_destroy_plan(p->forward);
	fftw_destroy_plan(p->backward);
	fftw_free(p->in); 
	fftw_free(p->out);
	p->in = p->out = NULL;
	p->size = 0;

	free(p);
}

int compute_fft(void *handle, double *inp)
{
	int  i;
	fft_plan_t *p = (fft_plan_t *)handle;
	if(!p)
	      return -1;
	if(!p->in)
	      return -1;
	if(!p->size)
	      return -1;

	for(i = 0; i < p->size; i++) {
		p->in[i][0] = inp[i];
		p->in[i][1] = 0;
	}
	fftw_execute(p->forward);
	return 0;
}
int compute_ifft(void *handle, double *inp)
{
	int  i;
	fft_plan_t *p = (fft_plan_t *)handle;
	if(!p)
	      return -1;
	if(!p->in)
	      return -1;
	if(!p->size)
	      return -1;

	for(i = 0; i < p->size; i++) {
		p->in[i][0] = inp[i];
		p->in[i][1] = 0;
	}
	fftw_execute(p->backward);
	return 0;
}


#define ind_real(p,i) p->out[i][0]
#define ind_imag(p,i) p->out[i][1]
#define SQR(val)     ((val) * (val))

void get_absolute(void *handle, double *out)
{
	int i;
	fft_plan_t *p = (fft_plan_t *)handle;
	for(i = 0; i < p->size; i++) {
		out[i] = sqrt(SQR(p->out[i][0]) + SQR(p->out[i][1]));
	}
}

void auto_correlation(void *handle, double *in, double *out)
{
	fft_plan_t *p = (fft_plan_t *)handle;
	compute_fft(p, in);
	get_absolute(p, out);
	compute_ifft(p, out);
	get_absolute(p, out);
}

