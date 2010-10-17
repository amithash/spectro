#ifndef __KMEANS_H_
#define __KMEANS_H_
#include "hist_dist.h"

typedef double (* dist_t)(hist_t *a, hist_t *b);
typedef void (*zero_t)(hist_t *a);
typedef void (*copy_t)(hist_t *out, hist_t *in);
typedef void (*cent_accum_t)(hist_t *out, hist_t *in);
typedef void (*cent_final_t)(hist_t *out, hist_t *in, unsigned int len);

typedef struct {
	dist_t dist;
	zero_t zero;
	copy_t copy;
	cent_accum_t caccum;
	cent_final_t cfinal;
} kmeans_ops_t;

typedef struct {
	hist_t *data;
	int id;
} clustered_data_t;



int hist_cluster(int km, hist_t *data, int len, kmeans_ops_t ops, clustered_data_t **clustered_out);

#endif
