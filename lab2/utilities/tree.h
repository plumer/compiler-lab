#ifndef __TREE_H__
#define __TREE_H__

#define FLAG_MULTI	 8


#define EXTDEF_GLOBAL_VARIABLES 16
#define EXTDEF_FUNCTION 17

#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "symbol.h"

struct _id_extra {
	char name[32];
	int is_definition;
	int defined_at;
};
typedef struct _id_extra id_extra_t;


struct tree_node {
	unsigned short flags;
	unit_t unit_name;
	int line;
	struct tree_node * children[8];
	int num_of_children;
// semantic area
	union {
		int i_value;
		float f_value;
		unsigned char relop_type;
		unsigned char pbc_part;
		id_extra_t * id_extra;
		struct {
			unsigned short exp_type_flag;
			union var_type exp_type;
			unsigned short is_lvalue;
		} exp_extra;
		int type_name;
	} extra_value;
#define i_value extra_value.i_value
#define f_value extra_value.f_value
#define id_extra extra_value.id_extra
#define relop_type extra_value.relop_type
#define pbc_part extra_value.pbc_part
#define exp_is_lvalue extra_value.exp_extra.is_lvalue
#define exp_type_flag extra_value.exp_extra.exp_type_flag
#define exp_type extra_value.exp_extra.exp_type
#define type_name extra_value.type_name
// end of semantic area
};

int add_child(struct tree_node *p, struct tree_node *c, int n);
struct tree_node * create_node(unit_t, int);
struct tree_node * create_end_node(unit_t);
struct tree_node * new_node();
int init_node(struct tree_node * n);
void print_tree(struct tree_node *root);

#endif
// __TREE_H__
