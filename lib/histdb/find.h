#ifndef __FIND_H_
#define __FIND_H_

#include "list.h"

Node *find(Node *head, char *dir_name, char **ext, int ext_len);
unsigned int find_len(Node *head);

#endif
