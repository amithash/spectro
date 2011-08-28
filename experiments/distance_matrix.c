#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <pthread.h>
#include "distance_matrix.h"
/*
 * for a n element distance matrix:
 *
 * Total entries = n * (n - 1) / 2
 *
 * Get Index k from i & j
 * ----------------------
 * k = (i * (i - 1) / 2) + j
 *
 * Get i & j from Index k
 * -----------------------
 * i = floor( ( 1 + sqrt( 1+8k ) ) / 2 )
 * j = k - (i * (i - 1) / 2)
 *
 * Thus the operation can be parallelized just like any other.
 */

void delete_dist_matrix(dist_matrix_t *mat)
{
	if(!mat)
	      return;
	if(mat->data)
	      free(mat->data);
	free(mat);
}

dist_matrix_t *new_dist_matrix(unsigned int n)
{
	dist_matrix_t *out;
	int len = (n * (n - 1)) / 2;

	out = malloc(sizeof(dist_matrix_t));
	if(!out)
	      return NULL;
	out->n = n;
	out->len = len;
	out->data = calloc(len, sizeof(float));
	return out;
}

float *ind_dist_matrix(dist_matrix_t *mat, int i, int j)
{
	int ind;
	if(!mat || !mat->data)
	      return NULL;

	if(i == j)
	      return NULL;
	if(j > i) {
		int temp = j;
		j = i;
		i = temp;
	}
	ind = ((i * (i - 1))/2) + j;
	return &mat->data[ind];
}

float get_dist_matrix(dist_matrix_t *mat, int i, int j)
{
	float *val = ind_dist_matrix(mat, i, j);
	if(!val)
	      return 0;
	return *val;
}

typedef struct {
	int start;
	int end;
	void *data;
	float *out;
	pthread_mutex_t *mutex;
	volatile int     *total;
	index_cb_t index;
	distance_cb_t dist;
} worker_data_t;

static void *worker_thread(void *priv)
{
	int i,j,k;
	int done = 0;
	worker_data_t *work = (worker_data_t *)priv;
	for(k = work->start; k < work->end; k++) {
		i = (int)((1.0 + sqrt(1.0 + 8.0 * k))/2);
		j = k - ((i * (i - 1)) / 2);
		work->out[k] = 
		    work->dist(work->index(work->data, i), work->index(work->data,j));
		done++;
		if(done % 10 == 0) {
			pthread_mutex_lock(work->mutex);
			*(work->total) += done;
			pthread_mutex_unlock(work->mutex);
			done = 0;
		}
	}
	if(done) {
		pthread_mutex_lock(work->mutex);
		*work->total += done;
		pthread_mutex_unlock(work->mutex);
	}
	pthread_exit(NULL);
}

dist_matrix_t *create_dist_matrix(
	void *data, unsigned int n, 
	distance_cb_t dist, 
	index_cb_t index,
	perc_cb_t perc_cb,
	int nr_threads)
{
	dist_matrix_t *mat;
	int i;
	int total_operations = (n * (n - 1)) / 2;
	int work_per_thread = 0;
	pthread_t *threads;
	worker_data_t *work_data;
	pthread_mutex_t mutex;
	volatile int total = 0;


	if(n < 2)
	      return NULL;

	if(nr_threads < 1)
	      nr_threads = 1;

	while((work_per_thread = total_operations / nr_threads) < 1) {
		nr_threads--;
	}
	printf("Performing operation using %d threads\n", nr_threads);

	mat = new_dist_matrix(n);
	if(!mat)
	      return NULL;

	threads = calloc(nr_threads, sizeof(pthread_t));
	if(!threads) {
		delete_dist_matrix(mat);
		return NULL;
	}

	work_data = calloc(nr_threads, sizeof(worker_data_t));
	if(!work_data) {
		free(threads);
		delete_dist_matrix(mat);
		return NULL;
	}

	pthread_mutex_init(&mutex, NULL);

	for(i = 0; i < nr_threads; i++) {
		work_data[i].index = index;
		work_data[i].dist = dist;
		work_data[i].out = mat->data;
		work_data[i].data = data;
		work_data[i].mutex = &mutex;
		work_data[i].total = &total;
		work_data[i].start = work_per_thread * i;
		if(i == nr_threads - 1) {
			work_data[i].end = total_operations;
		} else {
			work_data[i].end = work_per_thread * (i + 1);
		}
		pthread_create(&threads[i], 0, worker_thread, &work_data[i]);
	
	}
	while(perc_cb && total < total_operations) {
		float perc = ((float)total * 100) / (float)total_operations;
		perc_cb(perc);
		sleep(1);
	}
	if(perc_cb)
		perc_cb(100);
	for(i = 0; i < nr_threads; i++) {
		pthread_join(threads[i], NULL);
	}
	pthread_mutex_destroy(&mutex);
	free(work_data);
	free(threads);
	return mat;
}

void print_dist_matrix(dist_matrix_t *mat)
{
	int i,j;
	if(!mat || !mat->data)
	      return;
	for(i = 0; i < mat->n; i++) {
		for(j = 0; j < mat->n; j++) {
			printf("%.2f ", get_dist_matrix(mat, i, j));
		}
		printf("\n");
	}
}

#ifdef DISTANCE_MATRIX_TEST
void *index_cb(void *_data, int ind)
{
	float *data = (float *)_data;
	return (void *)&data[ind];
}

float dist_cb(void *_a, void *_b)
{
	float a,b;
	a = *((float *)_a);
	b = *((float *)_b);

	return sqrt((a - b) * (a - b));
}
int main(void)
{
	float arr[4] = {1.1,2.3,5.3,7.4};
	dist_matrix_t *mat = create_dist_matrix(arr, 4, dist_cb, index_cb, 2);
	if(!mat) {
		printf("Memory error!\n");
		return -1;
	}
	print_dist_matrix(mat);
	delete_dist_matrix(mat);
	return 0;
}

#endif
