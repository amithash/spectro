#include "hist_dist.h"
#include <pthread.h>
#include <float.h>

#define NMAX_DEFAULT 10
#define INIT_DIST DBL_MAX


int main(int argc, char *argv[])
{
	unsigned int len;
	int i;
	hist_t *hist_list;
	int ref_ind = -1;
	int maxes_len = NMAX_DEFAULT;
	similar_t *similar = NULL;

	if(argc < 3) {
		spect_error("USAGE: HIST_DB ref_spect_file.spect");
		exit(-1);
	}
	if(argc == 4) {
		maxes_len = atoi(argv[3]);
	}

	if(read_hist_db(&hist_list, &len, argv[1])) {
		spect_error("Could not read hist db: %s", argv[1]);
		exit(-1);
	}
	for(i = 0; i < len; i++) {
		if(strcmp(hist_list[i].fname, argv[2]) == 0) {
			printf("Found at index %d\n", i);
			ref_ind = i;
			break;
		}
	}
	if(ref_ind == -1) {
		spect_error("Cound not find %s in db",argv[2]);
		exit(-1);
	}

	if(get_most_similar(hist_list, len, ref_ind, maxes_len, &similar)) {
		spect_error("Malloc failed!\n");
		exit(-1);
	}

	printf("Distance\t%-40s%-30s%-30s\n","Title", "Artist", "Album");
	printf("---------------------------------------------------------------------------------------------------------------------------------\n");
	for(i = 0; i < maxes_len; i++) {
		if(similar[i].ind == -1) {
			printf("WAAAAAAAA\n");
		      continue;
		}

		printf("%f\t%-40s%-30s%-30s\n", similar[i].dist, 
					hist_list[similar[i].ind].title, 
					hist_list[similar[i].ind].artist, 
					hist_list[similar[i].ind].album);
	}
	free(hist_list);
	free(similar);

	return 0;
}
