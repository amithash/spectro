#ifndef __LIST_H_
#define __LIST_H_

typedef struct Node_s Node;

struct Node_s{
	char name[256];
	Node *next;
};

void list_purge(Node *head);
unsigned int list_len(Node *head);
int list_split(Node **out, unsigned int out_len, Node *head);

#endif
