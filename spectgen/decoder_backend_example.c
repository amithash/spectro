#include "decoder_backend.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <string.h>
/*
 * Other required includes
 */

struct decoder_example_handle {
	void *client_handle; /* Always the first element */
	pthread_t thread;
	/* Other handles required on a per-file/per-instance basis */
};

/* Required operation functions */
static int decoder_backend_example_start(void *_handle);
static int decoder_backend_example_close(void *_handle);
static int decoder_backend_example_open(void **_handle, void *client_handle, char *file);

/* The function table for this backend */
static struct decoder_backend_ops backend_ops = {
	decoder_backend_example_open,
	decoder_backend_example_close,
	decoder_backend_example_start
};

/* backend init/constructor
 *
 * This is where you will do all startup global initialization,
 * and register the file extensions you support for decoding.
 *
 * This function with the constructor attribute will be called
 * on decoder's start
 */
__attribute__((constructor))
void decoder_backend_example_init(void)
{
	/* All init routines */

	/* Register the file type extensions supported
	 * by this backend */
	decoder_backend_register(&backend_ops, "example");
}

/* backend deinit/destructor
 *
 * This is where we do all our deinits. If required.
 *
 * TODO: In the future you might have to deregister the
 * supported file extensions. But for now since this is
 * statically linked to the decoder, it is not needed.
 */
__attribute__((destructor))
void decoder_backend_example_exit(void)
{
	/* All global deinit function */
}

/* backend thread
 *
 * This is where the meat of the decoding happens.
 * This thread is created on a per file handle basis.
 *
 * Note you would probably have to do some extra 
 * work to convert to the floating point and mono 
 * format required by the front-end.
 *
 * Output SAMPLE FORMAT: 
 * each sample is a floating point variable 
 * ranging from -1.0 to 1.0.
 * Multi-channel samples must be converted to
 * mono. It is up to the implementer to choose
 * to use only 1 channel or mix all the channels.
 *
 * length is in terms of samples and not in terms
 * of buffer size.
 *
 * frate is in Hz. Example: 44100
 *
 * frate is mandatory to be present in the first push.
 * If for some reason, you cannot get the frate at 
 * the start of decoding, then you will be responsible
 * to buffer the entire file and push it to the front
 * once the frate is got.
 */
static void *decoder_backend_example_thread(void *_handle)
{
	struct decoder_example_handle *handle = 
	    (struct decoder_example_handle *)_handle;
	while(1) {
		float *buffer;
		unsigned int len;
		unsigned int frate;

		/* Get the frame rate of the input.
		 * for some libs, this might be 
		 * got from outside the loop. Some
		 * decoders might not even need this 
		 * loop.
		 */

		/* malloc buffer, decode into it a 
		 * chunk (or all)
		 */

		/* Update len to contain the number
		 * of samples in buffer
		 */

		/* Push the buffer to the front. 
		 * The front end is responsible to
		 * de-allocate the buffer. 
		 *
		 * frate is required for every push. 
		 * frate is _mandatory_ for the first
		 * push.
		 */
		decoder_backend_push(handle, buffer, len, frate);
	}

	/* Signal the frontend that there is no more
	 * samples to decode
	 */
	decoder_backend_push(handle, NULL, 0, 0);

	/* Return */
	pthread_exit(NULL);
}

/* =============================================================
 *	 	Front-end Visible Functions
 * ============================================================*/

/* file open
 *
 * This is the open callback. This is where you do all open
 * related operations to decode the file.
 *
 * handle	 out	The handle you provide to this instance.
 * client_handle in	The handle the client uses for this
 * 			instance.
 * file		 in	Name of the file to decode.
 *
 * Return: 0 on success, Negative error code on failure.
 */
static int decoder_backend_example_open(void **_handle, void *client_handle, char *file)
{
	struct decoder_example_handle *handle;

	*_handle = NULL;
	handle = (struct decoder_example_handle *)
	    malloc(sizeof(struct decoder_example_handle));

	if(!handle)
	      return -1;
	handle->client_handle = client_handle;

	/* Open file and perform other open related operations
	 * on the file */

	/* Populate other decoder specific instance variables
	 * in the decoder_*_handle struct
	 */

	return 0;
}

/* start decoding
 *
 * This function creates a thread to do the decoding.
 * This function may also perform all start related operations.
 *
 * handle	in	The handle allocated and got
 * 			from the open call.
 *
 * Return: 0 on success, Negative error code on failure
 */
static int decoder_backend_example_start(void *_handle)
{
	struct decoder_example_handle *handle = 
	    (struct decoder_example_handle *)_handle;
	int rc;

	if(!handle)
	      return -1;

	rc = pthread_create(&handle->thread, /* Pthread id */
			    0,               /* Any thread attributes */
			    /* Decoder thread to do the meat of the
			     * decoding */
			    decoder_backend_example_thread, 
			    handle /* Handle we pass to the thread to
				      associate itself to the instance */
			    );
	if(!rc) {
		return -1;
	}

	/* Note DO NOT wait for the thread. as the function says, this
	 * just kicks off the decoding.
	 */
	return 0;
}

/* close
 *
 * Wait for decoding to complete and close (deinit)
 * all handles associated with this instance.
 *
 * handle	in	the handle got by open.
 *
 * Return: 0 on success, Negative error code on failure.
 */
static int decoder_backend_example_close(void *_handle)
{
	struct decoder_example_handle *handle = 
	    (struct decoder_example_handle *)_handle;
	int rc;
	void *pars;

	if(!handle)
	      return -1;

	/* Check other private handles */

	/* Call this before performing any other 
	 * close operations. This makes sure you block close
	 * if any decoding is still being performed.
	 */
	pthread_join(handle->thread, &pars);

	/* Perform any other close related operations.
	 * Free any per-instance variables allocated.
	 */
	free(handle);

	return 0;
}

