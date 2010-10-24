#include "hist_dist.h"
#include <pthread.h>
#include <float.h>


typedef struct {
	hist_t *hist;
	double dist;
} max_type;

#define NMAX_DEFAULT 10
#define INIT_DIST DBL_MAX

max_type *maxes;

void init_nmax(int len)
{
	int i;
	maxes = (max_type *)calloc(len, sizeof(max_type));
	for(i = 0; i < len; i++) {
		maxes[i].hist = NULL;
		maxes[i].dist = INIT_DIST;
	}
}

int main(int argc, char *argv[])
{
	unsigned int len;
	int i,j;
	hist_t *hist_list;
	spect_t ref_spect;
	hist_t ref_hist;
	int maxes_len = NMAX_DEFAULT;

	if(argc < 3) {
		spect_error("USAGE: HIST_DB ref_spect_file.spect");
		exit(-1);
	}
	if(argc == 4) {
		maxes_len = atoi(argv[3]) + 1;
	}

	if(read_spectf(argv[2], &ref_spect) < 0) {
		spect_error("Could not read %s", ref_spect.fname);
		exit(-1);
	}
	spect2hist(&ref_hist, &ref_spect);

	
	free_spect(&ref_spect);

	if(read_hist_db(&hist_list, &len, argv[1])) {
		spect_error("Could not read hist db: %s", argv[1]);
		exit(-1);
	}

	init_nmax(maxes_len);

	for(i = 0; i < len; i++) {
		double idist = hist_distance(&ref_hist, &hist_list[i]);
		for(j = 0; j < maxes_len; j++) {
			if(idist < maxes[j].dist) {
				maxes[j].dist = idist;
				maxes[j].hist = &hist_list[i];
				break;
			}
		}
	}
	printf("Distance\tTitle\tArtist\tAlbum\n");
	for(i = 1; i < maxes_len; i++) {
		if(!maxes[i].hist) {
			printf("WAAAAAAAA\n");
		      continue;
		}

		printf("%f\t%s\t%s\t%s\n", maxes[i].dist, maxes[i].hist->title, maxes[i].hist->artist, maxes[i].hist->album);
	}
	free(hist_list);

	return 0;
}
