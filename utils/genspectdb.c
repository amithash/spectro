#include "spect.h"
#include <pthread.h>

#define NTHREADS_FREAD   4
#define NTHREADS_COMPUTE 2

int main(int argc, char *argv[])
{
	unsigned int len;
	spect_t *spect_list;
	int rc;

	if(argc != 3) {
		spect_error("USAGE: SPECT_FILE_LIST SPECT_DB_FILE_NAME");
		exit(-1);
	}

	printf("Read spect files\n");

	if((rc = read_spect_list(argv[1], &spect_list, &len, NTHREADS_FREAD)) < 0) {
		spect_error("Failed to read spect list file (rc=%d): %s",rc,argv[1]);
		exit(-1);
	}

	printf("Writing db\n");

	if(write_spect_db(argv[2], spect_list, len) < 0)  {
		spect_error("Write failed!\n");
		exit(-1);
	}
	free(spect_list);
	spect_list = NULL;

	free(spect_list);

	return 0;

}
