#include "hist.h"

int main(int argc, char *argv[])
{
	spect_t spect;
	hist_t hist;

	if(read_spectf(argv[1], &spect)) {
		spect_error("Reading %s failed", argv[1]);
		exit(-1);
	}

	spect2hist(&hist, &spect);
	free_spect(&spect);

	plot_hist(&hist);

	return 0;
}
