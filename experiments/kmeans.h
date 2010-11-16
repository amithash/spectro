#ifndef __KMEANS_H_
#define __KMEANS_H_

/*----------------------------------------------------------
 * 		Callback functions
 *--------------------------------------------------------*/

/* Take in pointers representing two points and return the
 * distance between them */
typedef float (* dist_t)(void *a, void *b);

/* Take in pointer representing the point and zero it */
typedef void (*zero_t)(void *a);

/* Copy point from out to in */
typedef void (*copy_t)(void *out, void *in);

/* Accumilate in to out. For a simple euclidian space,
 * this is just out[i] += in[i] */
typedef void (*accum_t)(void *out, void *in);

/* Fianlize the accumilated output (from accum_t) and put the results 
 * in out. For a simple euclidian space, this is just
 * in[i] /= len */
typedef void (*final_t)(void *out, void *in, unsigned int len);

/* Take in the pointer to the list, and return pointer of the
 * point indexed by 'ind' */
typedef void *(*index_t)(void *in, int ind);

/* allocate and return the pointer to the list of points */
typedef void *(*calloc_t)(int len);

/* Virtual table pointer for the callbacks mentioned above.
 * This allows kmeans to be performed on any kind of point */
typedef struct {
	dist_t dist;
	zero_t zero;
	copy_t copy;
	accum_t accum;
	final_t final;
	index_t index;
	calloc_t calloc;
} kmeans_ops_t;

/* Clustered output */
typedef struct {
	void *data; /* Pointer to the data point */
	int id;     /* The cluster id */
} clustered_data_t;

/* CLUSTER:
 * km		[in]	Number of required clusters
 * data		[in]	The pointer to the array of points
 * len		[in]	Length of data points in data
 * ops		[in]	The operation virtual function table
 * out		[out]	The pointer to the output data
 *
 * SIDE EFFECTS:
 * out is malloc-ed. It must be freed later once out is no longer required.
 *
 * NOTES:
 * the member of out[i] contains the id (The cluster [0,km)
 * data is the pointer to in[i]. Thus make sure not to over write/free
 * 'in' after calling the function as you end up overwriting/freeing the
 * data point.
 */
int cluster(int km, void *data, int len, kmeans_ops_t ops, clustered_data_t **clustered_out);

#endif
