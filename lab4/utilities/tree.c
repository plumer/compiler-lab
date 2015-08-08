#include "tree.h"
#include <string.h>

// utility function
void print_helper(struct tree_node *p, int d) {
	if (p == NULL) {
		return;
	}
	int i;
	for (i = 0; i < d; ++i) {
		printf("  ");
	}
	printf("%s (%d) ", unit_to_string(p -> unit_name) ,p->line);
	if (p -> unit_name == TOKEN_INTEGER) {
		printf("value: %d", p -> i_value);
	} else if (p -> unit_name == TOKEN_FLOAT) {
		printf("value: %f", p -> f_value);
	} else if (p -> unit_name == TOKEN_IDENTIFIER) {
		printf("name: %s", p -> id_extra -> name);
	}
/*	if (p -> num_of_children != 1)*/putchar('\n');
	
	if (p -> flags & FLAG_MULTI) {
		for (i = 0; i < 8; ++i) if (p -> children[i]) {
				print_helper(p -> children[i], d);
		}
	} else {
		for (i = 0; i < 8; ++i) if (p -> children[i]) {
			print_helper(p->children[i], d+1);
		}
	}
}

int add_child(
	struct tree_node * p, struct tree_node *c, int n) {
	if (p == NULL) return -1;
	if (c == NULL) return 1;
	if (n < 0 || n > 7) return 2;
	
	p -> children[n] = c;
	if (n+1 > p -> num_of_children)
		p -> num_of_children = n+1;
	return 0;
}

int init_node(struct tree_node *n) {
	memset(n, 0, sizeof(struct tree_node));
	return 0;
}

struct tree_node *
new_node() {
	return (struct tree_node *)malloc(sizeof(struct tree_node));
}

struct tree_node *
create_node(unit_t u, int l) {
	struct tree_node * ptr = create_end_node(u);
	ptr -> line = l;
	return ptr;
}

struct tree_node * create_end_node(unit_t u) {
	struct tree_node * ptr = (struct tree_node *)malloc(sizeof(struct tree_node));
	memset(ptr, 0, sizeof(struct tree_node));
	INIT_LIST_HEAD( &(ptr -> truelist) );
	INIT_LIST_HEAD( &(ptr -> falselist) );
	INIT_LIST_HEAD( &(ptr -> nextlist) );
	ptr -> unit_name = u;
	return ptr;
}

void print_tree(struct tree_node *p) {
	print_helper(p, 0);
}
