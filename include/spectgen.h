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

#ifndef __SPECTGEN_H_
#define __SPECTGEN_H_

#ifdef __cplusplus
	extern "C" {
#endif

/****************************************************************
 * THEORY OF OPERATION
 *
 * This module utilizes the pipelined parallel module of execution.
 * The decoding is done in its own thread context, while the
 * DFT computation is performed in another thread context. 
 * Thus the caller can operate on each band while the rest
 * of the system computes the next.
 *
 * The valid operation sequence is either:
 * spectgen_open
 * spectgen_start
 * spectgen_pull - Repeated till it returns NULL.
 * spectgen_frate - Optional.
 * spectgen_close
 *
 * or
 *
 * spectgen_open
 * spectgen_read
 * spectgen_frate - Optional
 * spectgen_close
 *
 * You can compute the spectrogram of multiple audio files in
 * parallel. 
 *
 ***************************************************************/

/** Enumeration type specifying the scale types which are supported
 * by the library for 'banding' frequencies.
 */
typedef enum {
	MEL_SCALE = 0,	/** Mel Scale */
	BARK_SCALE,     /** Bark Scale */
	SEMITONE_SCALE, /** 12 tones per octave - the semitone */
	MAX_SCALE
} scale_t;

/** Enumeration type specifying if the operation returns a spectrogram
 * or a cepstogram.
 */
typedef enum {
	SPECTOGRAM,
	CEPSTOGRAM
} spect_method_t;

/** Handle for an opened file */
typedef void *spectgen_handle;


/****************************************************************
 * FUNCTION: spectgen_open
 * 
 * DESCRIPTION: 
 * Opens an Audio file to generate the spectrogram.
 *
 * handle	out	The handle for this open
 * fname	in	Name of the audio file.
 * window_size	in	Size of each stream window on which
 * 			an DFT is performed.
 * step_size	in	Size by which the window is moved 
 * 			through the stream.
 * scale	in	The scale on which the DFT output
 * 			is grouped.
 * method	in	Either performing either a spectrogram
 * 			or a cepstogram. 
 * nbands	in/out	Input: The number of bands required.
 * 			Output: Adjusted number of bands if
 * 			the input is not possible.
 *
 * RETURN:
 * 0 on Success, Negative error code on failure.
 ***************************************************************/
int spectgen_open(spectgen_handle *handle, char *fname, 
		unsigned int window_size, unsigned int step_size, 
		scale_t scale, spect_method_t method, 
		unsigned int *nbands);

/****************************************************************
 * FUNCTION: spectgen_start
 * 
 * DESCRIPTION: 
 * Start generation of the spectrogram on the input audio file.
 *
 * handle	in	The handle for this context
 *
 * RETURN:
 * 0 on Success, Negative error code on failure.
 *
 * DEPENDENCIES:
 * spectgen_open must have been called.
 ***************************************************************/
int spectgen_start(spectgen_handle handle);

/****************************************************************
 * FUNCTION: spectgen_pull
 * 
 * DESCRIPTION: 
 * Returns the next band.
 *
 * handle	in	The handle for this context
 *
 * RETURN:
 * A pointer to a floating point buffer containing 'nbands'
 * number of power values for each band of frequencies.
 * NULL is returned when there are no more bands to return
 * (End of stream is reached).
 *
 * DEPENDENCIES:
 * spectgen_start must have been called.
 *
 * NOTES:
 * If spectgen is still working, this function will block
 * till a band is ready. Thus it is guaranteed that return
 * of NULL is always the end of the stream.
 ***************************************************************/
float *spectgen_pull(spectgen_handle handle);

/****************************************************************
 * FUNCTION: spectgen_read
 * 
 * DESCRIPTION: 
 * This is a wrapper on top of spectgen_start and spectgen_pull
 * This function will allocate and return the entire spectrogram
 * of the opened audio file.
 *
 * handle	in	The handle for this context
 * band_array	out	The pointer to return the allocated
 * 			and populated buffer region.
 * nbands	in	The number of bands required (This should
 * 			be the one returned by spectgen_open).
 *
 * RETURN:
 * The total length of the returned buffer region. Negative
 * if this call fails.
 *
 * DEPENDENCIES:
 * spectgen_open must have been called.
 *
 * NOTES:
 * This function will block till the entire spectrogram is ready.
 * Do no use this with spectgen_start and spectgen_pull.
 ***************************************************************/
int spectgen_read(spectgen_handle handle, float **band_array, 
			unsigned int nbands);


/****************************************************************
 * FUNCTION: spectgen_frate
 * 
 * DESCRIPTION: 
 * Returns the average frame rate of the stream.
 *
 * handle	in	The handle for an opened audio file.
 *
 * RETURN:
 * The average frame rate of the computed stream.
 *
 * NOTES:
 * Call this after spectgen_read() or the last got sample
 * from spectgen_pull(), that way the returned average
 * frame rate accurately represents the entire audio file.
 ***************************************************************/
unsigned int spectgen_frate(spectgen_handle _handle);


/****************************************************************
 * FUNCTION: spectgen_close
 * 
 * DESCRIPTION: 
 * Closes the audio file.
 *
 * handle	in	The handle for the opened audio file.
 *
 * RETURN:
 * 0 on Success, Negative error code on failure.
 ***************************************************************/
int spectgen_close(spectgen_handle handle);

#ifdef __cplusplus
	}
#endif 

#endif
