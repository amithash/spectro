#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "scale.h"

/* 
 * Reference:
 * http://www2.ling.su.se/staff/hartmut/bark.htm
 */

#define MAX_FREQ (20000)
#define MIN_FREQ (20)

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

static char *nothing = "";

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

typedef struct {
	unsigned int 	freq;
	float		coef;
} coef_t;

static const coef_t inv_equal_loudness_coef[34] = {
	    { 0,     0.5833333333333334 },
	    { 20,    0.6194690265486725 },
	    { 30,    0.6796116504854368 },
	    { 40,    0.7216494845360825 },
	    { 50,    0.7526881720430109 },
	    { 60,    0.7692307692307693 },
	    { 70,    0.7865168539325843 },
	    { 80,    0.8045977011494253 },
	    { 90,    0.813953488372093  },
	    { 100,   0.8235294117647058 },
	    { 200,   0.8974358974358975 },
	    { 300,   0.9210526315789473 },
	    { 400,   0.9210526315789473 },
	    { 500,   0.9210526315789473 },
	    { 600,   0.9210526315789473 },
	    { 700,   0.9090909090909092 },
	    { 800,   0.8974358974358975 },
	    { 900,   0.8805031446540881 },
	    { 1000,  0.8750000000000001 },
	    { 1500,  0.8860759493670887 },
	    { 2000,  0.9090909090909092 },
	    { 2500,  0.9459459459459461 },
	    { 3000,  0.9790209790209791 },
	    { 3700,  1.0                },
	    { 4000,  0.9929078014184397 },
	    { 5000,  0.9459459459459461 },
	    { 6000,  0.8860759493670887 },
	    { 7000,  0.8333333333333333 },
	    { 8000,  0.813953488372093  },
	    { 9000,  0.813953488372093  },
	    { 10000, 0.8235294117647058 },
	    { 12000, 0.7368421052631579 },
	    { 15000, 0.6363636363636364 },
	    { 20000, 0.5600000000000001 }
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

	for(i = sc_min + sc_step, j = 0; i <= sc_max + sc_step && j < nbands; i += sc_step, j++) {
		out[j] = func_f(i);
	}

	return out;
}

float *generate_scale_norm_table(unsigned *bark_bands, unsigned int nbands)
{
	float *out;
	int i, j = 1;

	if(!bark_bands)
	      return NULL;

	if(nbands < 2)
	      return NULL;

	/* Technically nbands can be anything, but it is nice to have sanity bounds on
	 * input parameters */
	if(nbands > 20000)
	      return NULL;
	out = calloc(nbands, sizeof(float));
	if(!out)
	      return NULL;
	for(i = 0; i < nbands; i++) {
		float x = (float)bark_bands[i];
		float x1, x2, y1, y2;
		int found = 0;
		for(; j < 34; j++) {
			x1 = inv_equal_loudness_coef[j-1].freq;
			x2 = inv_equal_loudness_coef[j].freq;
			y1 = inv_equal_loudness_coef[j-1].coef;
			y2 = inv_equal_loudness_coef[j].coef;
			if(x >= x1 && x <= x2) {
				found = 1;
				break;
			}
		}
		if(found == 0) {
			out[i] = 1;
			continue;
		}
		/* Linear interpolation */
		out[i] = ((x - x1) * (y2 - y1) / (x2 - x1)) + y1;
	}
	return out;
}

char *get_scale_name(scale_t scale)
{
	if(scale < 0 || scale >= MAX_SCALE)
	      return nothing;

	return scale_name[scale];
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
	printf("Entered scale %d, name = %s nbands=%d\n", (int)scale, get_scale_name(scale), nbands);
	bands = generate_scale_table(nbands, scale);
	for(i = 0; i < nbands; i++) {
		printf("%d\n", bands[i]);
	}
	return 0;
}
#endif
