#include "bpm.h"

int main(int argc, char *argv[])
{
	spect_t spect;
	int rc;
	double bpm[BPM_LEN];
	if(argc <= 1) {
		spect_error("USAGE: %s <spect file>", argv[0]);
		exit(-1);
	}

	if((rc = read_spectf(argv[1], &spect)) != RM_SUCCESS) {
		spect_error("Reading %s returned in error=%s",argv[1],RM_RC_STR(rc));
		exit(-1);
	}

	if(spect2bpm(bpm, &spect)) {
		spect_error("Conversion failed!");
		goto cleanup_spect;
	}

	plot_bpm(bpm);

cleanup_spect:
	free_spect(&spect);

	return 0;
}
