#include <assert.h>
#include <stdio.h>
#include "syntax.tab.h"
#include "utilities/tree.h"
#include "utilities/symbol.h"

extern int error;
extern struct tree_node * root;
extern const char * current_file;
extern const char * unit_str[];
extern const char * type_str[];
extern const char * compare_str[];

void semantic_analysis(struct tree_node *);
int check_expression(struct tree_node *);

// used by symtab_add_variable to handle arrays
struct array * construct_basic_array_type(struct tree_node *basicspec, struct tree_node *vardec);
struct array * construct_struct_array_type(struct struct_entry *se, struct tree_node *vardec);

// adds an entry in the variable and handles arrays,
// if spec is a struct specifier, it will only search if the struct exists
struct var_entry * symtab_add_variable(struct tree_node *spec, struct tree_node *vardec);

// assumption: the function is always a definition
// adds an entry in the function table, and add the parameters
struct func_entry * symtab_add_function(struct tree_node *fundec);

// adds an entry in the struct table, and
// if strpec contains a definition of the struct, add all the members
struct struct_entry * symtab_add_struct(struct tree_node *strspec);

// compares if two types are the same.
// on true, 1 is returned, else, 0 is returned.
int compare_type(
	unsigned short ltf, union var_type * lt,
	unsigned short rtf, union var_type * rt);

// compares if two structs are the same.
// if two structs has the same name, then they are same.
// else, if two structs has the same structure, then they are same.
// on true, 1 is returned, else, 0 is returned.
int compare_struct(struct struct_entry *left, struct struct_entry * right);

// compares is two arrays are the same.
// if the dimensions are not the same, then they are not same.
// else, if the basic types of them is not the same, then they are not same.
int compare_array(struct array *la, struct array *lb);

struct func_entry * function_field;

int push_anonymous_struct();
void set_anonymous_struct(struct struct_entry *);
struct struct_entry *get_anonymous_struct();
int pop_anonymous_struct();

