#ifndef _HIST_DIST_H_
#define _HIST_DIST_H_

#include "hist.h"

typedef struct {
	int ind;
	float dist;
} similar_t;

float hist_distance(hist_t *hist1, hist_t *hist2);
int get_most_similar(hist_t *list, unsigned int len, int this_i, int n, similar_t **_out);

#endif
