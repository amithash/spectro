#include <stdlib.h>
#include <stdio.h>

#include "plot.h"

#define error(fmt) fprintf(stderr, fmt)

const char *standard_types[PLOT_MAX] = {
	"points",
	"lines"
};

#define IND(arr,i,j,len) arr[(i * len) + j]

int plot(float *x, float *y, unsigned int ncol, unsigned int len, plot_type_t type, int debug)
{
	FILE *f;
	int i,j;

	if(type >= PLOT_MAX) {
		return -1;
	}

	if(!(f = fopen("__data.txt", "w"))) {
		error("Failed to create temp file\n");
		return -1;
	}
	for(i = 0; i < len; i++) {
		if(x)
		      fprintf(f, "%.4f %.4f", x[i], IND(y,0,i,len));
		else
		      fprintf(f, "%.4f", IND(y, 0, i, len));

		for(j = 1; j < ncol; j++)
			fprintf(f, " %.4f", IND(y,j,i,len));
		fprintf(f, "\n");
	}
	fclose(f);

	if(!(f = fopen("__out.gp", "w"))) {
		error("Failed to create gp script\n");
		return -1;
	}
	fprintf(f, "plot \\\n");
	for(i = 0; i < ncol; i++) {
		if(x)
			fprintf(f, "\"__data.txt\" using 1:%d with %s title \"data %d\"", i + 2, standard_types[type], i);
		else
			fprintf(f, "\"__data.txt\" using %d with %s title \"data %d\"", i + 1, standard_types[type], i);
		if(i < (ncol - 1)) {
			fprintf(f, ", \\");
		}
		fprintf(f, "\n");
	}
	fprintf(f, " pause -1\n");
	fclose(f);
	system("gnuplot __out.gp");
	if(!debug) {
		system("rm -f __out.gp __data.txt");
	}
}
#ifdef PLOT_TEST
int main(void)
{
	float x[4] = {1,2,3,4};
	float y[1][4] = {{1,2,3,4}};

	plot(x, (float *)y, 1, 4, PLOT_LINES);

	return 0;
}
#endif
