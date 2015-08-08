#ifndef __TREE_H__
#define __TREE_H__

#define FLAG_MULTI 8
#define FLAG_INT 16
#define FLAG_FLOAT 32
#define FLAG_ID 64

#include <stdio.h>
#include <stdlib.h>
#include "syntax.tab.h"

struct treeNode {
	int flags;
	char elementName[32];
	char idName[32];
	int line;
	union {
		int i_value;
		float f_value;
	} v;
#define i_value v.i_value
#define f_value v.f_value
	struct treeNode * children[8];
	int numOfChildren;
};

int addChild(struct treeNode *p, struct treeNode *c, int n);
struct treeNode * createNode(const char *, int);
struct treeNode * createEndNode(const char *);
struct treeNode * newNode();
int initNode(struct treeNode * n);
void printTree(struct treeNode *root);



#endif // __TREE_H__
