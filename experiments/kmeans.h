#ifndef __KMEANS_H_
#define __KMEANS_H_

typedef float (* dist_t)(void *a, void *b);
typedef void (*zero_t)(void *a);
typedef void (*copy_t)(void *out, void *in);
typedef void (*cent_accum_t)(void *out, void *in);
typedef void (*cent_final_t)(void *out, void *in, unsigned int len);
typedef void *(*index_t)(void *in, int ind);
typedef void *(*calloc_t)(int len);

typedef struct {
	dist_t dist;
	zero_t zero;
	copy_t copy;
	cent_accum_t caccum;
	cent_final_t cfinal;
	index_t index;
	calloc_t calloc;
} kmeans_ops_t;

typedef struct {
	void *data;
	int id;
} clustered_data_t;



int hist_cluster(int km, void *data, int len, kmeans_ops_t ops, clustered_data_t **clustered_out);

#endif
