#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "hist.h"

int main(int argc, char *argv[]) {
	hist_t hist;
	spect_t spect;
	int rc;
	FILE *pgf;
	char outf_name[256] = "./tmp.pgm";
	char buf[100];
	int i,j;
	double max_val[NBANDS];

	if(argc < 2) {
		printf("USAGE: %s FILE.spect4 (Optional output pgm file default: tmp.pgm)\n", argv[0]);
		exit(-1);
	}
	strcpy(spect.fname, argv[1]);
	if((rc = read_spect(&spect)) != 0) {
		printf("Reading spect file %s failed with rc=%d",argv[1], rc);
		exit(-1);
	}
	if(spect.valid != 1) {
		printf("%s is not a valid spect! :-(\n",argv[1]);
		exit(-1);
	}
	spect2hist(&hist, &spect);

	if(argc > 2) {
		strcpy(outf_name, argv[2]);
	}
	pgf = fopen(outf_name, "w");
	if(pgf == NULL) {
		printf("Creating %s failed!\n", outf_name);
		exit(-1);
	}
	sprintf(buf, "P5\n%d %d\n255\n", HIST_LEN, NBANDS);
	write(fileno(pgf), buf, strlen(buf) * sizeof(char));
	for(i = 0; i < NBANDS; i++) {
		max_val[i] = 0;
		for(j = 0; j < HIST_LEN; j++) {
			if(hist.spect_hist[i][j] > max_val[i]) {
				max_val[i] = hist.spect_hist[i][j];
			}
		}
		for(j = 0; j < HIST_LEN; j++) {
			unsigned int ui_val = (unsigned int)((hist.spect_hist[i][j] / max_val[i]) * 255);
			unsigned char val = (unsigned int)ui_val;
			if(write(fileno(pgf), &val, sizeof(unsigned char)) != sizeof(unsigned char)) {
				printf("Write error!\n");
			}
		}
	}
	fclose(pgf);

	return 0;
}

