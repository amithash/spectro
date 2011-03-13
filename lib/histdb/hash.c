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


