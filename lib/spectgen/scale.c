#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "scale.h"

#define MAX_FREQ (20000)
#define MIN_FREQ (100)

typedef float (*forward_t)(float);
typedef float (*reverse_t)(float);

static float mel2freq(float m);
static float freq2mel(float f);

static float bark2freq(float z);
static float freq2bark(float f);

static float semitone2freq(float s);
static float freq2semitone(float f);

static char *scale_name[MAX_SCALE] = {
	"BARK_SCALE",
	"MEL_SCALE",
	"SEMITONE_SCALE"
};

static forward_t forward[MAX_SCALE] = {
	bark2freq,
	mel2freq,
	semitone2freq
};

static reverse_t reverse[MAX_SCALE] = {
	freq2bark,
	freq2mel,
	freq2semitone
};


static float mel2freq(float m)
{
	return 700 * ( exp(m/1127) - 1 );
}

static float freq2mel(float f)
{
	return 1127 * log(1 + f / 700);
}

static float freq2bark(float f)
{
	f = f < 1 ? 1 : f;
	return (26.81 / (1 + 1960 / f )) - 0.53;
}

static float bark2freq(float z)
{
	return 1960 / (26.81 / (z + 0.53) - 1);
}

static float freq2semitone(float f)
{
	f = f < 1 ? 1 : f;
	return  12 * log(f / 127.09) / log(2);
}

static float semitone2freq(float s)
{
	return 127.09 * exp(s * log(2) / 12.0);
}

unsigned int *generate_scale_table(unsigned int nbands, scale_t scale)
{
	unsigned int *out;
	forward_t func_f;
	reverse_t func_r;
	float sc_max;
	float sc_min;
	float sc_step;
	float i;
	int j;

	if(scale < 0 || scale >= MAX_SCALE)
	      return NULL;
	if(nbands < 2)
	      return NULL;

	/* Technically nbands can be anything, but it is nice to have sanity bounds on
	 * input parameters */
	if(nbands > 20000)
	      return NULL;

	out = calloc(nbands, sizeof(unsigned int));
	if(!out)
	      return NULL;

	func_f = forward[scale];
	func_r = reverse[scale];

	sc_max = func_r(MAX_FREQ);
	sc_min = func_r(MIN_FREQ);
	sc_step = (sc_max - sc_min) / (float)(nbands);

	for(i = sc_min, j = 0; i <= sc_max; i += sc_step, j++) {
		out[j] = func_f(i);
	}

	return out;
}
#ifdef TEST_SCALE
int main(int argc, char *argv[])
{
	scale_t scale;
	int nbands = 24;
	int i;
	unsigned int *bands = NULL;

	if(argc < 2) {
		scale = BARK_SCALE;
	} else {
		int _s = atoi(argv[1]);
		if(_s < 0 || _s >= (int)MAX_SCALE) {
			printf("Invalid scale %d. Using 1 as BARK_SCALE\n", _s);
			scale = BARK_SCALE;
		} else {
			scale = (scale_t)_s;
		}
	}
	if(argc >= 3) {
		nbands = atoi(argv[2]);
		if(nbands < 1 || nbands > 1000) {
			printf("Unbounded number of bands!\n");
			exit(-1);
			return 0;
		}
	}
	printf("Entered scale %d, name = %s nbands=%d\n", (int)scale, scale_name[scale], nbands);
	bands = generate_scale_table(nbands, scale);
	for(i = 0; i < nbands; i++) {
		printf("%d\n", bands[i]);
	}
	return 0;
}
#endif
