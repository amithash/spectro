#include <stdio.h>
#include <stdlib.h>
#include "spectgen.h"
#include <unistd.h>

#define WINDOW_SIZE 4096
#define STEP_SIZE   WINDOW_SIZE / 2


int main(int argc, char *argv[])
{
	FILE *f;
	spectgen_handle handle;
	float *band;
	if(argc < 3) {
		printf("Usage: %s <Input MP3 File> <Output Spect File>\n", argv[0]);
		exit(-1);
	}
	f = fopen(argv[2], "w");
	if(!f) {
		printf("Failed to create %s\n", argv[2]);
		exit(-1);
	}

	if(spectgen_open(&handle, argv[1], WINDOW_SIZE, STEP_SIZE)) {
		printf("Spectgen open on %s failed\n", argv[1]);
		fclose(f);
		exit(-1);

	}
	if(spectgen_start(handle)) {
		printf("Start failed\n");
		goto start_failed;
	}
	while((band = spectgen_pull(handle)) != NULL) {
		if(write(fileno(f), band, sizeof(float) * 24) != (sizeof(float) * 24)) {
			printf("Write failed\n");
		}
		free(band);
	}
start_failed:
	spectgen_close(handle);
	fclose(f);
	return 0;
}
