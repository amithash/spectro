#include <stdio.h>
#include <unistd.h>

#include "../spect-config.h"

#define SEP " "

int main(void)
{
	int n = 0;
	unsigned char c;
	int ind = 0;
	while((n = read(fileno(stdin), &c, sizeof(unsigned char))) > 0) {
		printf("%d",((unsigned int)c) & 0xFF);

		if(++ind == NBANDS) {
			printf("\n");
			ind = 0;
		} else {
			printf(SEP);
		}
	}
	return 0;
}
