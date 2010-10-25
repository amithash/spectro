#include "hist_dist.h"
#include <float.h>

#define BEAT_DIST

/* Compute the hellinger distance of two pdfs a and b */
static double hdistance(double *a, double *b, unsigned int len) 
{
	double dist = 0.0;
	int i;
	if(a == NULL || b == NULL) {
		return 0;
	}

	for(i = 0; i < len; i++) {
		dist += sqrt(a[i] * b[i]);
	}
	return sqrt(1 - dist);
}

/* Compute the euclidian distance */
static double edistance(double *dist, unsigned int len) 
{
	int i;
	double val = 0;
	if(dist == NULL) {
		return 0;
	}

	for(i = 0; i < len; i++) {
		val += pow(dist[i], 2);
	}
	return sqrt(val / len);
}

double hist_distance(hist_t *hist1, hist_t *hist2)
{
	int col;
	double dist[NBANDS];
  double beat_dist[NBANDS];
	for(col = 0; col < NBANDS; col++) {
		dist[col] = hdistance(hist1->spect_hist[col],
				      hist2->spect_hist[col],
				      HIST_LEN);
	}
	for(col = 0; col < NBANDS; col++) {
		beat_dist[col] = hdistance(hist1->beats[col],
				      hist2->beats[col],
				      BEAT_LEN);
	}
#ifdef BEAT_DIST
  return edistance(beat_dist, NBANDS);
#else
	return edistance(dist, NBANDS);
#endif
}

void cent_clear(hist_t *hist)
{
	int i,j;
	hist->fname[0] = '\0';
	for(i = 0; i < NBANDS; i++) {
		for(j = 0; j < HIST_LEN; j++) {
			hist->spect_hist[i][j] = 0;
		}
	}
}

void cent_accum(hist_t *out, hist_t *in)
{
	int i,j;
	for(i = 0; i < NBANDS; i++) {
		for(j = 0; j < HIST_LEN; j++) {
			out->spect_hist[i][j] += sqrt(in->spect_hist[i][j]);
		}
	}
}

void cent_final(hist_t *out, hist_t *in, unsigned int len)
{
	int i,j;
	for(i = 0; i < NBANDS; i++) {
		for(j = 0; j < HIST_LEN; j++) {
			out->spect_hist[i][j] = pow(in->spect_hist[i][j] / len, 2);
		}
	}
}


