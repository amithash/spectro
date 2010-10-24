#include "spect.h"
#include <pthread.h>

#define NTHREADS_FREAD   4
#define NTHREADS_COMPUTE 2

int main(int argc, char *argv[])
{
	unsigned int len;
	int rc;

	if(argc != 3) {
		spect_error("USAGE: SPECT_FILE_LIST SPECT_DB_FILE_NAME");
		exit(-1);
	}

	if((rc = combine_spect_list(argv[1], argv[2], NTHREADS_FREAD)) < 0) {
		spect_error("Failed to read spect list file (rc=%d): %s",rc,argv[1]);
		exit(-1);
	}

	return 0;

}
