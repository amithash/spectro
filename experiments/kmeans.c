#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include "kmeans.h"

#define MAX_ITERS 40

static int naked_man(void *centroids, void *data, int cent_len, int data_len, kmeans_ops_t ops)
{
	int i,j,k;
	/* centroids selection = naked man's algo */
	printf("Choosing a centroid\n");

	ops.copy(ops.index(centroids,0), ops.index(data, rand() % data_len));

	for(i = 1; i < cent_len; i++) {
		double min_distance = DBL_MAX;
		void *point;
		for(k = 0; k < data_len; k++) {
			double total = 0;
			for(j = 0; j < i; j++) {
				double d = ops.dist(ops.index(data, k), ops.index(centroids, j));
				total += d > 0 ? (1 / pow(d,2)) : DBL_MAX;
			}
			if(total < min_distance) {
				min_distance = total;
				point = ops.index(data,k);
			}
		}
		if(!point) {
			return -1;
		}
		ops.copy(ops.index(centroids, i), point);
	}
	return 0;
}

static int kmeans(void *centroids, void *new_centroids, clustered_data_t *clustered, int len, int km,  kmeans_ops_t ops)
{
	int i,j,k;
	int changed = 0;
	unsigned int *lengths;

	lengths = calloc(km, sizeof(unsigned int));
	if(lengths == NULL) {
		return -1;
	}

	/* For each data point */
	for(i = 0; i < km; i++) {
		ops.zero(ops.index(new_centroids, i));
		lengths[i] = 0;
	}

	for(j = 0; j < len; j++) {
		/* assign it to the closest cluster */
		double min_distance = DBL_MAX;
		int clust = -1;
		for(k = 0; k < km; k++) {
			double distance = ops.dist(clustered[j].data, ops.index(centroids, k));
			if(distance < min_distance) {
				min_distance = distance;
				clust = k;
			}
		}
		if(clust != clustered[j].id) {
			clustered[j].id = clust;
			changed++;
		}
		lengths[clust]++;
		ops.accum(ops.index(new_centroids, clust), clustered[j].data);
	}
	for(k = 0; k < km; k++) {
		ops.final(ops.index(centroids, k), ops.index(new_centroids, k), lengths[k]);
	}
	return changed;
}


int cluster(int km, void *data, int len, kmeans_ops_t ops, clustered_data_t **clustered_out)

{
	int i;
	int iter;
	int errno = 0;
	clustered_data_t *clustered;
	void *centroids = ops.calloc(km);
	void *new_centroids = ops.calloc(km);
	if(centroids == NULL || new_centroids == NULL) {
		return -1;
	}

	if(km >= len ) {
		errno = -1;
		goto err1;
	}

	naked_man(centroids, data, km, len, ops);

	*clustered_out = clustered = calloc(len, sizeof(clustered_data_t));
	if(clustered == NULL) {
		printf("Malloc failed!\n");
		return -1;
	}
	for(i = 0; i < len; i++) {
		clustered[i].data = ops.index(data, i);
		clustered[i].id   = -1;
	}
	/* kmeans */
	printf("Starting kmeans!\n");
	for(iter = 0; iter < MAX_ITERS; iter++) {
		int changed;
		printf("Iter %d\n",iter);
		if((changed = kmeans(centroids, new_centroids, clustered, len, km, ops)) == 0) {
			break;
		}
		printf("%d changed groups... re-trying\n",changed);
	}

	printf("%d iterations done.\n", iter);

	if(iter == MAX_ITERS)  {
		errno = -2;
		goto err1;
	} else {
		errno = 0;
	}
	
err1:
	free(new_centroids);
	free(centroids);
	return errno;
}

