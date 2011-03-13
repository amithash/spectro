#ifndef __HASH__H_
#define __HASH__H_

#include "list.h"

void init_hash_table(Node **hash_table, unsigned int table_len);
void purge_hash_table(Node **hash_table, unsigned int table_len);
void add_entry(Node **hash_table, unsigned int table_size, char *name);
int exists_in_hash(Node **hash_table, unsigned int table_size, char *name);

#endif
