#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <histdb.h>
#include "distance_matrix.h"
#include "getopt_easy.h"
#include "plot.h"
#include "spect-config.h"

static const char *usage_str = "USAGE: %s -l <LIST FILE> -h <HDB FILE> [-o <OUT FILE>]\n";

typedef struct {
	char name[256];
} file_name_t;

float matrix_dist_cb(void *_a, void *_b)
{
	hist_t *a = (hist_t *)_a;
	hist_t *b = (hist_t *)_b;
	if(!a || !b)
	      return 0;
	return hist_distance(a, b, HELLINGER_DIVERGANCE);
}

void *matrix_index_cb(void *_hist_list, int ind)
{
	hist_t *hist_list = (hist_t *)_hist_list;
	return &hist_list[ind];
}

int populate_req_list(hist_t *out_list, unsigned int out_len, 
		      hist_t *in_list, unsigned int in_len,
		      file_name_t *file_list
			)
{
	int i,j;
	int ret = 0;

	for(i = 0; i < out_len; i++) {
		for(j = 0; j < in_len; j++) {
			if(strcmp(in_list[j].fname, file_list[i].name) == 0) {
				memcpy(&out_list[i], &in_list[j], sizeof(hist_t));
				ret++;
				break;
			}
		}
		if(j == in_len) {
			printf("Could not find %s in db\n", file_list[i].name);
		}
	}
	return ret; /* Returns number populated! */
}


int read_song_list(file_name_t **out, unsigned int *out_len, char *name)
{
	FILE *f;
	file_name_t *list = NULL;
	unsigned int list_len = 0;
	unsigned int max_list_len = 1024;


	f = fopen(name, "r");
	if(!f) {
		return -1;
	}

	list = malloc(sizeof(file_name_t) * max_list_len);
	if(!list) {
		return -2;
	}

	while(fgets(list[list_len].name, 256, f)) {
		int len = strlen(list[list_len].name);
		int i;
		for(i = len; i >= 0; i--) {
			if(list[list_len].name[i] == '\n')
			      list[list_len].name[i] = '\0';
		}
		list_len++;
		if(list_len == max_list_len) {
			max_list_len += 1024;
			list = realloc(list, sizeof(file_name_t) * max_list_len);
			if(!list) {
				return -2;
			}
		}
	}
	list = realloc(list, sizeof(file_name_t) * list_len);
	*out = list;
	*out_len = list_len;

	return 0;
}

void print_perc(float perc)
{
	progress(perc, stdout);
}

void convert_dist_matrix(float *out, dist_matrix_t *matrix)
{
	int i, j;
	if(!out || !matrix)
	      return;
	for(i = 0; i < matrix->n; i++) {
		for(j = 0; j < matrix->n; j++) {
			out[(i * matrix->n) + j] = 
			    get_dist_matrix(matrix, i, j);
		}
	}
}

int main(int argc, char *argv[])
{
	char *list_file = NULL;
	char *hdb_file = NULL;
	char *pgm_out = "a.pgm";
	hist_t *hist_list = NULL;
	unsigned int hist_len = 0;
	file_name_t *file_list = NULL;
	unsigned int file_len = 0;
	hist_t *req_list = NULL;
	dist_matrix_t *matrix;
	float *selfsim = NULL;
	int rc;


	getopt_easy_opt_t opt[] = {
		{"l:", STRING, (void *)&list_file},
		{"h:", STRING, (void *)&hdb_file},
		{"o:", STRING, (void *)&pgm_out}
		/* Possibly others */
	};

	if((rc = getopt_easy(&argc, &argv, opt, 3))) {
		printf("Options parsing failed rc = %d\n", rc);
		exit(-1);
	}

	if(!list_file || !hdb_file) {
		printf(usage_str, argv[1]);
		exit(-1);
	}

	if(read_histdb(&hist_list, &hist_len, hdb_file)) {
		printf("Opening hdb:%s failed!\n", hdb_file);
		exit(-1);
	}
	if(read_song_list(&file_list, &file_len, list_file)) {
		free(hist_list);
		printf("Opening list:%s failed!\n", list_file);
		exit(-1);
	}
	req_list = calloc(file_len, sizeof(hist_t));
	if(!req_list) {
		printf("MALLOC FAILED!\n");
		exit(-1);
	}
	if(populate_req_list(req_list, file_len, hist_list, hist_len, file_list) != file_len) {
		printf("Population failed!");
		exit(-1);
	}
	printf("Req list populated!!!\n"); fflush(stdout);
	free(hist_list);
	free(file_list);
	hist_list = req_list;
	hist_len = file_len;

	matrix = create_dist_matrix(hist_list, hist_len, 
				matrix_dist_cb,
				matrix_index_cb,
				print_perc,
				2
				);
	free(hist_list);
	selfsim = calloc(hist_len * hist_len, sizeof(float));
	if(!selfsim) {
		printf("MALLOC FAILURE!");
		exit(-1);
	}
	convert_dist_matrix(selfsim, matrix);
	delete_dist_matrix(matrix);
	
	pgm(pgm_out, selfsim, hist_len, hist_len, BACKGROUND_BLACK, GREYSCALE, ALL_NORMALIZATION);
	free(selfsim);

	return 0;
	
	
}
