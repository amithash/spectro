#ifndef _DECODER_I_H_
#define _DECODER_I_H_

#include "queue.h"
#include "decoder.h"

struct decoder_handle_struct
{
	void *backend_handle; /* Populated by backend */
	void *backend_info;   /* Populated by backend AL */
	q_type *queue;
};

#endif
