#ifndef __SCALE_H_
#define __SCALE_H_

typedef enum {
	MEL_SCALE = 0,
	BARK_SCALE,
	SEMITONE_SCALE,
	MAX_SCALE
} scale_t;

unsigned int *generate_scale_table(unsigned int nbands, scale_t scale);

#endif
