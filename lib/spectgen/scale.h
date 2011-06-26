#ifndef __SCALE_H_
#define __SCALE_H_
#include "spectgen.h"

unsigned int *generate_scale_table(unsigned int nbands, scale_t scale);
char *get_scale_name(scale_t scale);
float *generate_scale_norm_table(unsigned *bark_bands, unsigned int nbands);

#endif
