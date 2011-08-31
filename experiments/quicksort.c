#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include "quicksort.h"

/*
 * function quicksort('array')
      create empty lists 'less' and 'greater'
      if length('array') ≤ 1
          return 'array'  // an array of zero or one elements is already sorted
      select and remove a pivot value 'pivot' from 'array'
      for each 'x' in 'array'
          if 'x' ≤ 'pivot' then append 'x' to 'less'
          else append 'x' to 'greater'
      return concatenate(quicksort('less'), 'pivot', quicksort('greater'))
*/

static int seeded = 0;

static int tries = 0;

static inline void *get_index(void *_data, int ind, size_t e_size)
{
	uint8_t *data = (uint8_t *)_data;
	return data + (e_size * ind);
}

void *quicksort(void *data, unsigned int len, size_t e_size, qs_compare_cb_t compare)
{
	void *less = NULL;
	void *greater = NULL;
	int less_len = 0;
	int greater_len = 0;
	int i;
	int piviot = 0;
	void *d;
	void *piviot_element;

	if(!seeded) {
		srand ( time(NULL) );
		seeded = 1;
	}

	if(len <= 1)
	      return data;
	piviot = rand() % len;

	d = malloc(e_size * len);
	if(!d)
	      return data;

	less = d;
	greater = get_index(d, len, e_size);
	piviot_element = get_index(data, piviot, e_size);

	for(i = 0; i < len; i++) {
		tries++;
		if(i == piviot)
		      continue;
		if(compare(get_index(data, i, e_size), piviot_element)) {
			memcpy(get_index(less, less_len, e_size), get_index(data, i, e_size), e_size);
			less_len++;
		} else {
			greater_len++;
			memcpy(get_index(d, len - greater_len, e_size), get_index(data, i, e_size), e_size);
		}
	}
	memcpy(get_index(d, less_len, e_size), piviot_element, e_size);
	memcpy(data, d, e_size * len);

	less = data;
	greater = get_index(data, len - greater_len, e_size);
	free(d);

	less = quicksort(less, less_len, e_size, compare);
	greater = quicksort(greater, greater_len, e_size, compare);

	return data;
}

#ifdef TEST_QUICKSORT
int compare_cb(void *_a, void *_b)
{
	int *a = (int *)_a;
	int *b = (int *)_b;
	if(*a < *b)
	      return 1;
	return 0;
}

void print_arr(int *arr, int len)
{
	int i;
	for(i = 0; i < len; i++) {
		printf("%d ", arr[i]);
	}
	printf("\n");
}

int main(int argc, char *argv[])
{
	int i;
	int *in;
	int *out;
	int len = 20;
	if(argc > 1)
	      len = atoi(argv[1]);
	if(len < 1)
	      len = 20;

	in = malloc(sizeof(int) * len);
	for(i = 0; i < len; i++) {
		in[i] = rand() % 100;
	}
	print_arr(in, len);

	out = (int *)quicksort(in, len, sizeof(int), compare_cb);
	print_arr(out, len);
	printf("Tries = %d\n", tries);

	
	return 0;
}
#endif
