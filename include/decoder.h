/*******************************************************************************
    This file is part of spectro
    Copyright (C) 2011  Amithash Prasad <amithash@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*******************************************************************************/

#ifndef _DECODER_H_
#define _DECODER_H_

#ifdef __cplusplus
	extern "C" {
#endif

/****************************************************************
 * THEORY OF OPERATION
 *
 * This module utilizes the pipelined parallel model of execution.
 * Each call to decoder_init() will create a decoding thread
 * context and return a handle to that thread.
 * Thus, the caller can operate on a partially decoded stream
 * while the decoder is working on decoding the next stream.
 *
 * Each thread context can decode only 1 audio file at a time.
 * If multiple files needs to be decoded in parallel, then
 * you need to call decoder_init() multiple times and open
 * multiple files in their respective context.
 *
 * The valid operation is:
 * 1. decoder_init();
 *
 * 2. decoder_open(audio_file);
 * 3. decoder_data_pull() - Repeat till len = 0 is returned.
 * 4. decoder_close();
 *
 * Repeat steps 2,3 & 4 for each audio file.
 * 
 * 5. decoder_exit();
 *
 * Performing any action out of order specified above
 * _will_ result in undefined behavior.
 *
 * The library can be queried at the start to get a list
 * of supported audio file extensions. Opening an 
 * unsupported audio file will fail.
 *
 ***************************************************************/


/** Handle type for the decoder */
typedef void *decoder_handle;

/****************************************************************
 * FUNCTION: decoder_init
 * 
 * DESCRIPTION: 
 * Initialize a decoding thread context.
 *
 * handle	out	The handle for this context
 *
 * RETURN:
 * 0 on Success, Negative error code on failure.
 *
 * NOTES:
 * If multiple threads are involved, each thread should call
 * decoder_init and must operate only on this handle.
 ***************************************************************/
int decoder_init(decoder_handle *handle);

/****************************************************************
 * FUNCTION: decoder_open
 * 
 * DESCRIPTION: 
 * Opens an Audio file for decoding.
 *
 * handle	in	The handle for this context
 * fname	in	The name of the audio file.
 *
 * RETURN:
 * 0 on Success, Negative error code on failure.
 ***************************************************************/
int decoder_open(decoder_handle handle, char *fname);

/****************************************************************
 * FUNCTION: decoder_start
 * 
 * DESCRIPTION: 
 * Starts decoding a previously opened audio file.
 *
 * handle	in	The handle for this context
 *
 * RETURN:
 * 0 on Success, Negative error code on failure.
 *
 * DEPENDENCIES:
 * decoder_init, and decoder_open must have been called.
 ***************************************************************/
int decoder_start(decoder_handle handle);

/****************************************************************
 * FUNCTION: decoder_data_pull
 * 
 * DESCRIPTION: 
 * Get a section of the decoded stream as floating point numbers
 * (0 - 1.0). A Return len = 0 implies that the stream has 
 * ended.
 *
 * handle	in	The handle for this context
 * buffer	out	Allocated buffer storage containing
 * 			the decoded stream.
 * len		out	The number samples in this stream.
 * frate	out	The frame rate of all the samples
 * 			in this stream.
 *
 * RETURN:
 * None.
 *
 * DEPENDENCIES:
 * decoder_init, decoder_open and decoder_start must have been 
 * called. The caller is responsible to free the buffer
 * when done. 
 *
 * NOTES:
 * This function call will block if the decoder is still
 * working. Thus a return len of 0 is guaranteed to be the
 * end of the stream.
 ***************************************************************/
void decoder_data_pull(decoder_handle handle, float **buffer, 
			unsigned int *len, unsigned int *frate);

/****************************************************************
 * FUNCTION: decoder_close
 * 
 * DESCRIPTION: 
 * Closes an audio file.
 *
 * handle	in	The handle for this context
 *
 * RETURN:
 * 0 on Success, Negative error code on failure.
 *
 * DEPENDENCIES:
 * decoder_init, and decoder_open must have been called.
 * All the audio samples has been pulled using 
 * decoder_data_pull.
 *
 * closing before pulling all data will corrupt the context.
 * The next open _will_ contain this stream!
 ***************************************************************/
int decoder_close(decoder_handle handle);

/****************************************************************
 * FUNCTION: decoder_exit
 * 
 * DESCRIPTION: 
 * De-initialize a decoding thread context.
 *
 * handle	in	The handle for this context
 *
 * RETURN:
 * None
 *
 * NOTES:
 * If multiple threads are involved, each thread should call
 * decoder_exit on their respective handles before exiting.
 ***************************************************************/
void decoder_exit(decoder_handle handle);


/****************************************************************
 * FUNCTION: decoder_supported_extensions
 * 
 * DESCRIPTION: 
 *
 *
 * extensions	in/out	The array of character pointers
 * 			to place the output extension
 * 			strings.
 * out_len	in/out	Input is the length of the
 * 			extensions array, output is
 * 			the total length of the array
 * 			which was populated.
 *
 * RETURN:
 * None
 *
 * NOTES:
 * The caller must allocate extensions and also set up
 * the array of pointers (This is NOT a 2D character array!).
 ***************************************************************/
void decoder_supported_extensions(char **extensions, 
			unsigned int *out_len);

#ifdef __cplusplus
	}
#endif 

#endif
