#include "tree.h"
#include <string.h>

// utility function
void printHelper(struct treeNode *p, int d) {
	if (p == NULL) {
		return;
	}
	int i;
	for (i = 0; i < d; ++i) {
		printf("  ");
	}
	printf("%s (%d) ",p->elementName,p->line);
	if (p -> flags & FLAG_INT) {
		printf("value: %d", p -> i_value);
	} else if (p -> flags & FLAG_FLOAT) {
		printf("value: %f", p -> f_value);
	} else if (p -> flags & FLAG_ID) {
		printf("name: '%s'", p -> idName);
	}
/*	if (p -> numOfChildren != 1)*/putchar('\n');
	
	if (p -> flags & FLAG_MULTI) {
		for (i = 0; i < 8; ++i) if (p -> children[i]) {
				printHelper(p -> children[i], d);
		}
	} else {
		for (i = 0; i < 8; ++i) if (p -> children[i]) {
			printHelper(p->children[i], d+1);
		}
	}
}

int addChild(
	struct treeNode * p, struct treeNode *c, int n) {
	if (p == NULL) return -1;
	if (c == NULL) return 1;
	if (n < 0 || n > 7) return 2;
	
	p -> children[n] = c;
	if (n+1 > p -> numOfChildren)
		p -> numOfChildren = n+1;
	return 0;
}

int initNode(struct treeNode *n) {
	memset(n, 0, sizeof(struct treeNode));
	return 0;
}

struct treeNode *
new_node() {
	return (struct treeNode *)malloc(sizeof(struct treeNode));
}

struct treeNode *
createNode(const char * str, int l) {
	struct treeNode * ptr = createEndNode(str);
	ptr -> line = l;
	return ptr;
}

struct treeNode * createEndNode(const char *str) {
	struct treeNode * ptr = (struct treeNode *)malloc(sizeof(struct treeNode));
	memset(ptr, 0, sizeof(struct treeNode));
	strncpy(ptr -> elementName, str, strlen(str));
	return ptr;
}

void printTree(struct treeNode *p) {
	printHelper(p, 0);
}
