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

#ifndef __GENHISTDB_H_
#define __GENHISTDB_H_

#ifdef __cplusplus
	extern "C" {
#endif
#include "spect-config.h"

/****************************************************************
 * THEORY OF OPERATION
 *
 * This library deals with all aspects of the spectral histogram.
 * From creating, reading and using, it does everything.
 *
 * This module also utilizes the pipelined parallel model but
 * mostly from its use of spectgen and decoder libs. So think
 * of this library as the sink of pipes originating from the
 * previous two stages.
 *
 *
 *
 ***************************************************************/

/** Flag specifying whether a histdb will be updated or 
 * recreated */
typedef enum {
	UPDATE_MODE = 0, /** Update the histdb if newer files are added */
	RECREATE_MODE    /** Recreate the entire histdb */
} generate_mode_t;

/** Enumeration type specifying all kinds of 
 * builtin distance computation routines */
typedef enum {
	DISTANCE_START = 0,
	KL_DIVERGANCE = 0,
	JEFFREYS_DIVERGANCE,
	EXPECTED_VALUE_DIFFERENCE,
	EXPECTED_DIFFERENCE,
	K_DIVERGANCE,
	HELLINGER_DIVERGANCE,
	EUCLIDEAN_DISTANCE,
	JENSEN_DIVERGANCE,
	DISTANCE_END
} hist_dist_func_t;

/** Callback type to compute the distance between two vectors */
typedef float (*distance_func_t)(float *a, float *b, unsigned int len);

/** Distance type, includes a function call, the name and type 
 * This is mostly required for presentation reasons. */
typedef struct {
	hist_dist_func_t 	func_type;
	char             	name[32];
	distance_func_t		func;
} dist_t;

/** The file layout of each histdb entry. */
typedef struct {
	char fname[FNAME_LEN]; 				/* Absolute path of the audio file */
	char title[TITLE_LEN];				/* The title of the music file */
	char artist[ARTIST_LEN];			/* Artist of the music file */
	char album[ALBUM_LEN];				/* Album of the music file */
	unsigned int track;				/* Track number */
	unsigned int length;				/* Length in seconds of the audio */
	int exclude;					/* Flag: Mostly used during playback */
	float spect_hist[NBANDS][SPECT_HIST_LEN];	/* The spectral histogram of the audio
							   track */
} hist_t;

/****************************************************************
 * FUNCTIONS RELATING TO DISTANCE COMPUTATION
 ***************************************************************/

/****************************************************************
 * FUNCTION: hist_distance
 * 
 * DESCRIPTION: 
 * Compute the distance between two hist entries.
 *
 * hist1	in	First histogram entry.
 * hist2	in	Second histogram entry.
 * distance	in	Distance methodology used.
 *
 * RETURN:
 * The distance between the two hist entries.
 ***************************************************************/
float hist_distance(hist_t *hist1, hist_t *hist2, hist_dist_func_t distance);

/****************************************************************
 * FUNCTION: hist_distance_ext
 * 
 * DESCRIPTION: 
 * Compute the distance between two hist entries. By explicitly
 * specifying to compute your own distance. 
 * (Mostly present for experimentation).
 *
 * hist1	in	First histogram entry.
 * hist2	in	Second histogram entry.
 * func		in	Callback function to compute the dist.
 *
 * RETURN:
 * The distance between the two hist entries.
 ***************************************************************/
float hist_distance_ext(hist_t *hist1, hist_t *hist2, distance_func_t func);

/****************************************************************
 * FUNCTION: get_supported_distances
 * 
 * DESCRIPTION: 
 * Return a list of supported distance functions along
 * with their discretions.
 *
 * dist		out	Array of dist information structure.
 *
 * RETURN:
 * None
 *
 * NOTES:
 * Do not free or write into the array. It is only present 
 * to provide information. 
 ***************************************************************/
void get_supported_distances(dist_t **dist);

/****************************************************************
 * FUNCTION: hist_get_similar
 * 
 * DESCRIPTION: 
 * Gets the most similar hist entries to the required hist
 * entry.
 *
 * list		in	The hist list to look for.
 * len		in	The length of the hist list.
 * this_i	in	The array index of the hist entry for 
 * 			which the most similar entries are
 * 			required.
 * this_i_len   in      The length of the above array.
 * n		in	The n most similar entries.
 * ind		out	The list to populate the index values
 * 			of the n most entries.
 * dist		out	The respective distance between the
 * 			returned hist entry (at index) to
 * 			the required entry (at this_i).
 * dist_type	in	The distance function type to use
 * 			to compute the distances.
 * 			
 * RETURN:
 * 0 on success, Negative error value on failure.
 ***************************************************************/
int hist_get_similar(
	hist_t *list, unsigned int list_len, 
	unsigned int *this_i, unsigned int this_i_len,
	int n, int *ind, float *dist,
	hist_dist_func_t dist_type);

/****************************************************************
 * FUNCTION: hist_get_similar_ext
 * 
 * DESCRIPTION: 
 * Gets the most similar hist entries to the required hist
 * entry. This uses a user specified distance function 
 * rather the one of the inbuilt functions.
 *
 * list		in	The hist list to look for.
 * len		in	The length of the hist list.
 * this_i	in	The array of index of the hist entry for 
 * 			which the most similar entries are
 * 			required.
 * this_i_len   in      The length of the above array
 * n		in	The n most similar entries.
 * ind		out	The list to populate the index values
 * 			of the n most entries.
 * dist		out	The respective distance between the
 * 			returned hist entry (at index) to
 * 			the required entry (at this_i).
 * func		in	The distance function to call to
 * 			compute the distances.
 * 			
 * RETURN:
 * 0 on success, Negative error value on failure.
 ***************************************************************/
int hist_get_similar_ext(
	hist_t *list, unsigned int len,
	unsigned int *this_i, unsigned int this_i_len, /* Input */
	int n, int *ind, float *dist,              /* Output */
	distance_func_t func);

/****************************************************************
 * FUNCTIONS RELATING TO HISTDB READING AND WRITING
 ***************************************************************/

/****************************************************************
 * FUNCTION: read_histdb
 * 
 * DESCRIPTION: 
 * Reads a histdb file and returns an array containing the
 * entries.
 *
 * hist		out	Pointer to place the read array.
 * len		out	Length of the hist array.
 * fname	in	Name of the histdb file.
 *
 * RETURN:
 * 0 on success, Negative error code on failure.
 *
 * NOTES:
 * The caller is responsible to free the hist array.
 ***************************************************************/
int read_histdb(hist_t **hist, unsigned int *len, char *fname);

/****************************************************************
 * FUNCTION: read_append_histdb
 * 
 * DESCRIPTION: 
 * Reads a histdb file and appends it (Realloc) to an existing
 * hist array.
 *
 * hist		in/out	Pointer of an existing hist array to
 * 			append to.
 * len		in/out	Input: The length of the hist array
 * 			before start.
 * 			Output: The length after read and 
 * 			appending.
 * fname	in	Name of the histdb file.
 *
 * RETURN:
 * 0 on success, Negative error code on failure.
 *
 * NOTES:
 * The caller is responsible to free the hist array.
 * Specifying len = 0 and hist = NULL has the same effect
 * as calling read_histdb.
 ***************************************************************/
int read_append_histdb(hist_t **hist, unsigned int *len, char *fname);

/****************************************************************
 * FUNCTION: write_histdb
 * 
 * DESCRIPTION: 
 * Writes a hist array into a histdb file.
 *
 * hist		in	Hist array.
 * len		in	Number of entries in the hist array.
 * fname	in	Name of the file to write into.
 *
 * RETURN:
 * 0 on success, Negative error code on failure.
 ***************************************************************/
int write_histdb(hist_t *hist, unsigned int len, char *fname);

/****************************************************************
 * FUNCTIONS RELATING TO GENERATION OF A HIST LIST
 ***************************************************************/

/** Handle of the genhistdb call */
typedef void *genhistdb_handle_type;


/****************************************************************
 * FUNCTION: gen_hist
 * 
 * DESCRIPTION: 
 * Generates (and allocates space for) a hist entry
 *
 * fname	in	Name of the audio file for which the
 * 			hist entry is required.
 *
 * RETURN:
 * The allocated and populated hist entry, NULL on failure.
 ***************************************************************/
hist_t *gen_hist(char *fname);

/** Notification callback type. This is called by histdb lib
 * with the percent of completion.
 */
typedef void (*genhistdb_notification_type)(void *priv, int perc);

/****************************************************************
 * FUNCTION: generate_histdb_prepare
 * 
 * DESCRIPTION: 
 * Prepares to generate the hist list for all audio files in a
 * specified directory.
 *
 * handle	out	The handle which is associated with 
 * 			this context.
 * dirname	in	The name of the directory to search
 * 			music files for.
 * dbname	in	The name of the db file to write the
 * 			generated histdb.
 * nr_threads	in	The total number of files which are
 * 			processed in parallel.
 * mode		in	Either update (db is read and then
 * 			updated if any files are added
 * 			or deleted in the directory) or
 * 			re-created (Db if it exists, will
 * 			be overwritten). Update will not
 * 			re-compute for entries which are
 * 			already present.
 *
 * RETURN:
 * 0 on success, Negative error code on failure.
 *
 * NOTES:
 * nr_threads is a bit misleading, as the computation of
 * each audio file itself is parallelized (Pipelined model).
 * So think of it as the number of audio files processed
 * in parallel and not as the actual number of threads created.
 ***************************************************************/
int generate_histdb_prepare(genhistdb_handle_type *handle, char *dirname, char *dbname, 
			unsigned int nr_threads, generate_mode_t mode);

/****************************************************************
 * FUNCTION: generate_histdb_start
 * 
 * DESCRIPTION: 
 * Starts the generation of the histdb in its own thread context
 * and the caller is free to do any operations in the meantime.
 * The notification callback is called to inform the caller
 * on the progress.
 *
 * handle	in	Handle returned by the prepare call.
 * cb		in	The callback to call notifying on the
 * 			progress.
 * priv		in	a private cookie which will be passed
 * 			back in the notification function.
 * perc		in	Indicates that the caller wishes to
 * 			be notified every 'perc' percentage 
 * 			of the progress.
 *
 * RETURN:
 * 0 on Success, Negative error code on failure.
 *
 * NOTES:
 * The call to cb with perc = 100 indicates that the operation
 * has been completed.
 ***************************************************************/
int generate_histdb_start(genhistdb_handle_type _handle, 
			genhistdb_notification_type cb, void *priv, int perc);



#ifdef __cplusplus
	}
#endif 

#endif
