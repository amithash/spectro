
#ifndef __FDWT_H_
#define __FDWT_H_
typedef enum
{
	DWT_FORWARD = 0,
	DWT_REVERSE
} dwt_dir_t;

typedef void * dwt_plan_t;

void dwt_destroy_plan(dwt_plan_t plan);


/* Note:
 * if in_vec == out_vec, then an in-place transform is performed. During the transform's
 * execution, the contents of in_vec is undefined.
 * Else, extra buffers are created in order not to touch in_vec. This is a bit extra
 * processing in terms of a memcpy before the transform starts and after the transform
 * ends. Also extra memory is used to create the temp bufs.
 */
dwt_plan_t dwt_create_plan(int size, float *in_vec, float *out_vec, dwt_dir_t dir);



void dwt_execute(dwt_plan_t plan);

#endif

