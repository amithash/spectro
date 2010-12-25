#ifndef _PLOT_H_
#define _PLOT_H_

typedef enum {
	PLOT_POINTS = 0,
	PLOT_LINES,
	PLOT_MAX
} plot_type_t;

int plot(float *x, float *y, unsigned int ncol, unsigned int len, plot_type_t type, int debug);

#endif
