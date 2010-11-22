#include "hist_dist.h"
#include <float.h>

static float bcoefficient(float *a, float *b, unsigned int len)
{
	float dist = 0.0;
	int i;
	if(a == NULL || b == NULL) {
		return 0;
	}

	for(i = 0; i < len; i++) {
		dist += sqrt(a[i] * b[i]);
	}
	return dist;
}

/* Compute the bhattacharya distance of two pdfs a and b */
static float bdistance(float *a, float *b, unsigned int len) 
{
	return -1 * log(bcoefficient(a,b,len));
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
	return sqrt(val);
}

float hist_distance(hist_t *hist1, hist_t *hist2)
{
	int col;
	float dist_spect[NBANDS];

	for(col = 0; col < NBANDS; col++) {
		dist_spect[col] = bdistance(hist1->spect_hist[col],
				            hist2->spect_hist[col],
				            SPECT_HIST_LEN);
	}
	return edistance(dist_spect, NBANDS);
}

