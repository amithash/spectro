#include "kmeans.h"
#include "hist_dist.h"
#include "hist_ops.h"
#include <math.h>
#include <string.h>

#define DBNAME "./db.hdb"

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



	ops.dist = hist_distance;
	ops.zero = cent_clear;
	ops.copy = cent_copy;
	ops.caccum = cent_accum;
	ops.cfinal = cent_final;

	if(read_hist_db(&list, &len, argv[1])) {
		printf("ERROR Reading db!\n");
		exit(-1);
	}
	errno = hist_cluster(km, list, len, ops, &out);

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
				out[j].data->fname[255] = '\0';
				printf("%s\n",out[j].data->fname);
			}
		}
	}
	free(out);
	free(list);

	return 0;
}
	








