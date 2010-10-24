#include "bspect.h"

int main(int argc, char *argv[])
{
	spect_t spect;
	double beats[BEAT_LEN];
	int i;
	spect_t new_spect;
	if(argc != 2) {
		spect_error("Usage: %s <SPECT FILE", argv[0]);
		exit(-1);
	}
	if(read_spectf(argv[1], &spect)) {
		spect_error("Could not read %s",argv[1]);
		exit(-1);
	}
	compute_bspect(beats, &spect, argv[1]);

	for(i = 0; i < BEAT_LEN; i++) {
		fprintf(stderr,"%f,%f\n",TIME_MIN + (TIME_WIDTH * (double)i), beats[i]);
	}

	return 0;
}
