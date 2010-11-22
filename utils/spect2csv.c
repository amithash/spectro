#include <stdio.h>
#include <unistd.h>

#include "../spect-config.h"

#define SEP " "

#define PRINT_FORMAT "%f"
#define TRANSFORM(val) val

int main(void)
{
	int n = 0;
	float c;
	int ind = 0;
	while((n = read(fileno(stdin), &c, sizeof(float))) > 0) {
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
