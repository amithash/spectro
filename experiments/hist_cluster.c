#include <stdio.h>
#include <stdlib.h>
#include "kmeans.h"
#include "histdb.h"
#include "hist_ops.h"
#include <math.h>
#include <string.h>

#define DBNAME "../Music.hdb"

#define ONE 1267
#define TWO 7653
#define THR 5432

int main(int argc, char *argv[])
{
	hist_t *list;
	unsigned int len;
	kmeans_ops_t ops;
	clustered_data_t *out;
	int i,j;
	int errno;
	int km = 10;
	if(argc > 2) {
		printf("Arg %s\n",argv[2]);
		km = atoi(argv[2]);
	}
	if(km <= 0) {
		km = 10;
	}

	printf("Using %d\n",km);



	ops.dist = hist_dist;
	ops.zero = cent_clear;
	ops.copy = cent_copy;
	ops.accum = cent_accum;
	ops.final = cent_final;
	ops.calloc = hist_calloc;
	ops.index = hist_ind;

	if(read_histdb(&list, &len, argv[1])) {
		printf("ERROR Reading db!\n");
		exit(-1);
	}
	errno = cluster(km, list, len, ops, &out);

	if(errno != 0) {
		printf("Something went wrong\n");
		if(errno == -2) {
			printf("Max iterations reached!\n");
		} else {
			exit(-1);
		}
	}

	for(i = 0; i < km; i++) {
		printf("\n\n----------------------------------------\n");
		printf("                  %d                    \n",i);
		printf("----------------------------------------\n");
		for(j = 0; j < len; j++) {
			if(out[j].id == i) {
				hist_t *dt = (hist_t *)out[j].data;
				dt->fname[255] = '\0';
				printf("%s\n",dt->fname);
			}
		}
	}
	free(out);
	free(list);

	return 0;
}
	








