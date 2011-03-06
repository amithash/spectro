#include <stdio.h>
#include <stdlib.h>
#include "histdb.h"

int main(int argc, char *argv[])
{
	unsigned int nr_threads = 0;
	char *dirname;
	char *dbname;

	if(argc < 3) {
		printf("Usage: %s <Music Dir> <Out File>\n",argv[0]);
		return -1;
	}
	dirname = argv[1];
	dbname  = argv[2];
	if(argc > 3)
		nr_threads = atoi(argv[3]);
	printf("Starting!\n"); fflush(stdout);

	if(generate_histdb(dirname, dbname, nr_threads, UPDATE_MODE)) {
		printf("Generating histdb failed\n");
	}

	return 0;
}
