#include "hist_dist.h"
#include <float.h>

/* Compute the hellinger distance of two pdfs a and b */
static float hdistance(float *a, float *b, unsigned int len) 
{
	float dist = 0.0;
	int i;
	if(a == NULL || b == NULL) {
		return 0;
	}

	for(i = 0; i < len; i++) {
		dist += sqrt(a[i] * b[i]);
	}
	return sqrt(1 - dist);
}

/* Compute the scaled euclidian distance from the origin */
static float edistance(float *dist, unsigned int len) 
{
	int i;
	float val = 0;
	if(dist == NULL) {
		return 0;
	}

	for(i = 0; i < len; i++) {
		val += pow(dist[i], 2);
	}
	return sqrt(val / len);
}

/* Compute the distance between two song signatures */
float hist_distance(hist_t *hist1, hist_t *hist2)
{
	int col;
	float dist_spect[NBANDS];
	float dist_ceps[NBANDS/2];
	for(col = 0; col < NBANDS; col++) {
		dist_spect[col] = hdistance(hist1->spect_hist[col],
				            hist2->spect_hist[col],
				            SPECT_HIST_LEN);
	}
	for(col = 0; col < NBANDS/2; col++) {
		dist_ceps[col] = hdistance(hist1->ceps_hist[col],
				           hist2->ceps_hist[col],
				           CEPS_HIST_LEN);
	}
	return 
	    sqrt(pow(edistance(dist_spect, NBANDS),2) + pow(edistance(dist_ceps, NBANDS/2), 2));
}

