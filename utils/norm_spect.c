#include "spect_bounds.h"
#include <stdio.h>
#include <stdlib.h>

/* All values will be between 0 and 1 */
void normalize_spect(spect_t *spect, float bounds[NBANDS][2])
{
	int i,j;
	for(i = 0; i < NBANDS; i++) {
		float *bound = bounds[i];
		for(j = 0; j < spect->len; j++) {
			float val = spect->spect[i][j];

			val = val < bound[0] ? bound[0] : val;
			val = val > bound[1] ? bound[1] : val;
			spect->spect[i][j] = (val - bound[0]) / (bound[1] - bound[0]);
		}
	}
}

off_t current_pos(int fd)
{
	return lseek(fd, 0, SEEK_CUR);
}
void goto_pos(int fd, off_t off)
{
	lseek(fd, off, SEEK_SET);
}

void print_bounds(float bounds[NBANDS][2])
{
	int i;
	for(i = 0; i < NBANDS; i++) {
		float *bound = bounds[i];
		printf("[%.4f,%.4f]\n",bound[0], bound[1]);
	}
}

int main(int argc, char *argv[]) 
{
	float bounds[NBANDS][2];
	unsigned int len;
	FILE *fp;
	int fd;
	int i;
	spect_t spect;
	if(argc <= 1) {
		spect_error("USAGE: %s <spect db>", argv[0]);
		exit(-1);
	}
	printf("Getting bounds on input\n");
	if(spect_bounds(argv[1], bounds)) {
		spect_error("Getting bounds failed!");
		exit(-1);
	}
	spect_bounds_exit();

	print_bounds(bounds);

	printf("Processing\n");

	if((fp = fopen(argv[1], "r+")) == NULL) {
		spect_error("Open failed on %s",argv[1]);
		exit(-1);
	}
	fd = fileno(fp);

	if(read_spect_db_len(fd, &len)) {
		spect_error("Getting len failed");
		exit(-1);
	}

	for(i = 0; i < len; i++) {
		off_t pos = current_pos(fd);
		progress(100.0 * (float)i / (float)len);
		if(read_spect(fd, &spect)) {
			spect_error("Read record %d failed",i);
			exit(-1);
		}
		goto_pos(fd, pos);
		normalize_spect(&spect, bounds);
		if(write_spect(fd, &spect)) {
			spect_error("Write record %d failed", i);
			exit(-1);
		}
		fflush(fp);
		free_spect(&spect);
	}
	progress(100.0);

	return 0;
}
