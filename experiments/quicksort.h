#ifndef _QUICKSORT_H_
#define _QUICKSORT_H_

typedef int (*qs_compare_cb_t)(void *a, void *b); /* Acending: Return 1 if a < b return 1 else return false */

void *quicksort(void *data, unsigned int len, size_t e_size, qs_compare_cb_t compare);

#endif
