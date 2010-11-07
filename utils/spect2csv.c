#include <stdio.h>
#include <unistd.h>

#include "../spect-config.h"

#define SEP " "

#if 0
typedef unsigned char spect_e_type;
#define PRINT_FORMAT "%d"
#define TRANSFORM(val) (((unsigned int)(val))&0xff)
#endif

typedef float spect_e_type;
#define PRINT_FORMAT "%f"
#define TRANSFORM(val) val

int main(void)
{
	int n = 0;
	spect_e_type c;
	int ind = 0;
	while((n = read(fileno(stdin), &c, sizeof(spect_e_type))) > 0) {
		printf(PRINT_FORMAT, TRANSFORM(c));

		if(++ind == NBANDS) {
			printf("\n");
			ind = 0;
		} else {
			printf(SEP);
		}
	}
	return 0;
}
