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
#include "list.h"

void list_purge(Node *head)
{
	Node *node;
	Node *to_free;

	node = head;
	while(node) {
		to_free = node;
		node = node->next;
		free(to_free);
	}
}

unsigned int list_len(Node *head)
{
	unsigned int len = 0;
	Node *node = head;
	while(node) {
		len++;
		node = node->next;
	}
	return len;
}

int list_split(Node **out, unsigned int out_len, Node *head)
{
	unsigned int len;
	int i,j;
	unsigned int per_len;
	Node *node;
	Node *to_add;

	if(!out || !head)
	      return -1;

	for(i = 0; i < out_len; i++) {
		out[i] = NULL;
	}

	len = list_len(head);
	node = head;
	if(len <= out_len) {
		for(i = 0; i < len; i++) {
			out[i] = node;
			node = node->next;
			out[i]->next = NULL;
		}
		return 0;
	}
	per_len = len / out_len;
	for(i = 0; i < out_len; i++) {
		for(j = 0; j < per_len; j++) {
			to_add = node;
			node = node->next;
			to_add->next = out[i];
			out[i] = to_add;
		}
		if(i == out_len - 1) {
			while(node) {
				to_add = node;
				node = node->next;
				to_add->next = out[i];
				out[i] = to_add;
			}
		}
	}
	return 0;
}

void list_print(Node *head)
{
	Node *node;
	for(node = head; node; node = node->next) {
		printf("%s\n",node->name);
	}
}
