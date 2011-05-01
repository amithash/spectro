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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "list.h"
#include "hash.h"

void init_hash_table(Node **hash_table, unsigned int table_len)
{
	int i;
	for(i = 0; i < table_len; i++)
	      hash_table[i] = NULL;
}

void purge_hash_table(Node **hash_table, unsigned int table_len)
{
	int i;
	for(i = 0; i < table_len; i++){
		if(hash_table[i]) {
			list_purge(hash_table[i]);
			hash_table[i] = NULL;
		}
	}
}

static unsigned int get_idx(char *name, int max_ind)
{
	unsigned int idx = 0;
	int i;
	int len = strlen(name);
	for(i = 0; i < len; i++) {
		idx += (unsigned int)name[i];
	}
	return idx % max_ind;
}

void add_entry(Node **hash_table, unsigned int table_size, char *name)
{
	Node *node;
	unsigned int idx;
	node = malloc(sizeof(Node));
	if(!node) {
		printf("Malloc failed\n");
		return;
	}
	strcpy(node->name, name);
	idx = get_idx(node->name, table_size);
	node->next = hash_table[idx];
	hash_table[idx] = node;
}

int exists_in_hash(Node **hash_table, unsigned int table_size, char *name)
{
	Node *node;
	unsigned int idx;

	idx = get_idx(name, table_size);
	node = hash_table[idx];
	while(node) {
		if(strcmp(node->name, name) == 0) {
			return 1;
		}
		node = node->next;
	}
	return 0;
}


