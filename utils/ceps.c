#include <stdio.h>
#include <math.h>

#include <fftw3.h>
#include "ceps.h"

#define PRE_SCALE 1000

typedef struct
{
	float **ceps;
	int nrows;
	int ncols;
} ceps_t;


void free_ceps(ceps_t *ceps)
{
	int i;
	if(!ceps)
	      return;
	if(!ceps->ceps)
	      return;

	for(i = 0; i < ceps->nrows; i++) {
		if(ceps->ceps[i])
		      free(ceps->ceps[i]);
	}
	free(ceps->ceps);
	ceps->ceps = NULL;
}

int alloc_ceps(ceps_t *ceps, int len)
{
	int i;
	ceps->nrows = len;
	ceps->ncols = (NBANDS / 2);

	ceps->ceps = (float **)malloc(sizeof(float *) * ceps->nrows);
	if(!ceps->ceps)
	      return -1;
	memset(ceps->ceps, 0, sizeof(float *) * ceps->nrows);

	for(i = 0; i < ceps->nrows; i++) {
		ceps->ceps[i] = malloc(sizeof(float) * ceps->ncols);
		if(!ceps->ceps[i]) {
			free_ceps(ceps);
			return -1;
		}
	}
	return 0;
}

int spect2ceps(ceps_t *out, spect_t *in)
{
	float t_spect_in[NBANDS];
	fftwf_complex t_spect_out[NBANDS/2 + 1];
	fftwf_plan plan;
	int i,j;
	if(!out || !in)
		return -1;

	if(alloc_ceps(out, in->len))
		return -1;

	plan = fftwf_plan_dft_r2c_1d(NBANDS, t_spect_in, t_spect_out, FFTW_ESTIMATE);

	for(i = 0; i < in->len; i++) {

		/* copy array also perform log operation */
		for(j = 0; j < NBANDS; j++) {
			t_spect_in[j] = in->spect[j][i] <= 0 ? 1 : (in->spect[j][i] * 1000) + 1;
			t_spect_in[j] = log(t_spect_in[j]);
		}

		fftwf_execute(plan);

		/* Some processing will be required */
		for(j = 1; j < out->ncols + 1; j++) {
			float re = t_spect_out[j][0] / NBANDS;
			float im = t_spect_out[j][1] / NBANDS;
			float abs = (re * re) + (im * im);
			out->ceps[i][j-1] = abs;
		}
	}

	fftwf_destroy_plan(plan);

	return 0;
}

#define CEPS_BIN_WIDTH ((CEPS_MAX_VAL - CEPS_MIN_VAL) / CEPS_HIST_LEN)
#define FLOOR_MIN2(val) ((val) < CEPS_MIN_VAL ? CEPS_MIN_VAL : (val))
#define CEIL_MAX2(val)  ((val) >= CEPS_MAX_VAL ? (CEPS_MAX_VAL - CEPS_BIN_WIDTH) : (val))
#define NUM2BIN2(val)    (unsigned int)(FLOOR_MIN2(CEIL_MAX2(val)) / CEPS_BIN_WIDTH)
#define PREC 0.000001
#define FLOAT_EQUAL(val1, val2) (((val1) < (val2) + PREC) && ((val1) > (val2) - PREC))

int is_vec_zero(float *vec, int len)
{
	int i;
	for(i = 0; i < len; i++) {
		if(!FLOAT_EQUAL(vec[i], 0.0))
		      return 0;
	}
	return 1;
}

void ceps2chist(float chist[NBANDS/2][CEPS_HIST_LEN], ceps_t *ceps)
{
	int i,j;
	for(i = 0; i < NBANDS/2; i++) {
		for(j = 0; j < CEPS_HIST_LEN; j++) {
			chist[i][j] = 0;
		}
	}
	for(i = 0; i < ceps->nrows; i++) {
		if(is_vec_zero(ceps->ceps[i], ceps->ncols)) {
			continue;
		}
		for(j = 0; j < ceps->ncols; j++) {
			unsigned int ind = NUM2BIN2(ceps->ceps[i][j]);
			chist[j][ind]++;
		}
	}
	for(i = 0; i < NBANDS/2; i++) {
		float total = 0;
		for(j = 0; j < CEPS_HIST_LEN; j++) {
			total += chist[i][j];
		}
		if(total <= 0)
		      continue;
		for(j = 0; j < CEPS_HIST_LEN; j++) {
			chist[i][j] /= total;
		}
	}
}

int spect2chist(float hist[NBANDS/2][CEPS_HIST_LEN], spect_t *spect)
{
	ceps_t ceps;
	if(spect2ceps(&ceps, spect)) {
		return -1;
	}
	ceps2chist(hist, &ceps);
	free_ceps(&ceps);
	return 0;
}

