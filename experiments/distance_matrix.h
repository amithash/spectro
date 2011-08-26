#ifndef _DISTANCE_MATRIX_H_
#define _DISTANCE_MATRIX_H_

typedef float (*distance_cb_t)(void *a, void *b);
typedef void *(*index_cb_t)(void *data, int ind);

typedef struct {
	unsigned int n;
	unsigned int len;
	float *data;
} dist_matrix_t;

dist_matrix_t *new_dist_matrix(unsigned int n);
void delete_dist_matrix(dist_matrix_t *mat);
float *ind_dist_matrix(dist_matrix_t *mat, int i, int j);
float get_dist_matrix(dist_matrix_t *mat, int i, int j);
dist_matrix_t *create_dist_matrix(
	void *data, unsigned int n, 
	distance_cb_t dist, 
	index_cb_t index,
	int nr_threads);
void print_dist_matrix(dist_matrix_t *mat);


#endif
