#include "hist.h"

int main(int argc, char *argv[]) {
	if(argc != 3) {
		printf("USAGE: %s <SPECT DB FILE> <OUT HIST DB FILE>\n",argv[0]);
		exit(-1);
	}
	if(spectdb2histdb(argv[1], argv[2])) {
		printf("ERROR in converting %s to %s\n",argv[1], argv[2]);
	}
	return 0;
}
