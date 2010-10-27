#include "hist_dist.h"
#include <float.h>

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
	for(col = 0; col < NBANDS; col++) {
		dist[col] = hdistance(hist1->spect_hist[col],
				      hist2->spect_hist[col],
				      HIST_LEN);
	}
	return edistance(dist, NBANDS);
}

