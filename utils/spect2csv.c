#include <stdio.h>
#include <unistd.h>

#define SEP ","

#ifndef NBANDS
#define NBANDS 24
#endif

int main(void)
{
	int n = 0;
	unsigned char c;
	int ind = 0;
	while((n = read(fileno(stdin), &c, sizeof(char))) > 0) {
		printf("%d", (unsigned int)c & 0xFF);

		if(++ind == NBANDS) {
			printf("\n");
			ind = 0;
		} else {
			printf(SEP);
		}
	}
	return 0;
}
