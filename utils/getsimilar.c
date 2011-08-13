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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "histdb.h"
#include <pthread.h>
#include <dirent.h>
#include <sys/stat.h>
#include <float.h>
#include "getopt_easy.h"

#define NMAX_DEFAULT 10

int get_music_file_index(hist_t *list, unsigned int list_len, char *mp3)
{
	int i;
	char mp3_path[PATH_MAX] = "";
	int ret = -1;


	if(!list || !list_len || !mp3)
	      return -1;

	if(!realpath(mp3, mp3_path)) {
		printf("Could not get real path\n");
		return -1;
	}
	for(i = 0; i < list_len; i++) {
		if(strcmp(list[i].fname, mp3_path) == 0) {
			ret = i;
			break;
		}
	}
	return ret;
}

int main(int argc, char *argv[])
{
	unsigned int len;
	int i;
	hist_t *hist_list;
	unsigned int *ref_ind = NULL;
	int maxes_len = NMAX_DEFAULT;
	int *ind_p;
	float *dist_p;
	dist_t *sup_dist = NULL;
	hist_dist_func_t dist_func = DISTANCE_START;
	char *hist_db_name = NULL;
	unsigned int num_music_files = 0;
	int opt_get_dist = 0;
	int opt_dist_type = (int)HELLINGER_DIVERGANCE;
	getopt_easy_opt_t opt[] = {
		{"p", FLAG, &opt_get_dist},
		{"d:", INT, &opt_dist_type},
		{"n:", INT, &maxes_len},
		{"h:", STRING, (void *)&hist_db_name}
	};

	if(getopt_easy(&argc, &argv, opt, 4)) {
		printf("Option parsing failed!\n");
		exit(-1);
	}

	get_supported_distances(&sup_dist);
	if(opt_get_dist) {
		for(i = DISTANCE_START; i < DISTANCE_END; i++) {
			printf("[%d] %s\n", i, sup_dist[i].name);
		}
		exit(0);
	}
	if(!hist_db_name) {
		printf("Mandatory option: -h which specifies the db name\n");
		exit(-1);
	}
	if(opt_dist_type < (int)DISTANCE_START || opt_dist_type >= DISTANCE_END) {
		printf("Invalid distance: Try %s -p\n", argv[0]);
		exit(-1);
	}
	dist_func = (hist_dist_func_t)opt_dist_type;

	if(argc <= 1) {
		printf("USAGE: HIST_DB OPTIONS -h DB_PATH <music file> [<music file> <music file> ...]\n");
		printf("OPTIONS: -n - number of files, -d distance type, -p print supported distances\n");
		exit(-1);
	}
	num_music_files = argc - 1;

	printf("Getting similar tracks using \"%s\" as the distance function\n", sup_dist[dist_func].name);

	ind_p = calloc(maxes_len, sizeof(int));
	dist_p = calloc(maxes_len, sizeof(float));
	if(!ind_p || !dist_p) {
		printf("Malloc failure!\n");
		exit(-1);
	}

	if(read_histdb(&hist_list, &len, hist_db_name)) {
		printf("Could not read hist db: %s\n", hist_db_name);
		exit(-1);
	}

	ref_ind = (unsigned int *)calloc(num_music_files, sizeof(unsigned int));
	if(!ref_ind) {
		printf("Malloc failure\n");
		exit(-1);
	}

	for(i = 0; i < num_music_files; i++) {
		int ind = get_music_file_index(hist_list, len, argv[1 + i]);
		if(ind < 0) {
			printf("Could not find %s in the db\n", argv[1 + i]);
			exit(-1);
		}
		ref_ind[i] = (unsigned int)ind;
	}

	if(hist_get_similar(hist_list, len, ref_ind, num_music_files, maxes_len, 
			ind_p, dist_p, dist_func)) {
		printf("Malloc failed!\n");
		exit(-1);
	}

	printf("Distance\t%-40s%-30s%-30s\n","Title", "Artist", "Album");
	printf("---------------------------------------------------------------------------------------------------------------------------------\n");
	for(i = 0; i < maxes_len; i++) {
		if(ind_p[i] == -1) {
			printf("WAAAAAAAA\n");
		      continue;
		}

		printf("%f\t%-40s%-30s%-30s\n", dist_p[i], 
					hist_list[ind_p[i]].title, 
					hist_list[ind_p[i]].artist, 
					hist_list[ind_p[i]].album);
	}
	free(hist_list);
	free(ind_p);
	free(dist_p);

	return 0;
}
