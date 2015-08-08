#ifndef __var_H__
#define __var_H__

#define SYM_VAR	16
#define SYM_FUN	17
#define SYM_STR 18

#define PARAM_MAX 16
#define MEMBER_MAX 32

#define NAME_LEN 32
#include "common.h"
#include "list.h"

struct array;
struct struct_entry;

union var_type {
	unsigned basic_type;				// TYPE_INT or TYPE_FLOAT
	struct array * array_type;			// points to array
	struct struct_entry * struct_type;	// points to struct_entry
};


struct array {
	unsigned int num_of_elements;		// 10 for int a[10]
	unsigned int element_size;			// single element width, in number of bytes
	unsigned short type_flag;			// what's the element? basic, array or struct?
	unsigned short dimension;			// always greater than 0
	union var_type elem;
};


// the variable entry type definition
struct var_entry {
	char name[NAME_LEN];			// identifier name
	unsigned short type_flag;		// is it basic, array or a struct
	union var_type t;				// see union var_type
	unsigned short is_definition;
	unsigned short size;
	unsigned int defined_at;		// the line where it is declared at
	struct list_head list_ptr;		// points to the next variabe entry
};

/**
 * initialize variable table.
 * should be used when a new .cmm code file is about to be analyzed.
 */
void init_var_table();

/**
 * clear CURRENT variable table.
 */
void clear_var_table();

/**
 * add a variable entry in the CURRENT variable table.
 * if a variable with the same name in the variable table exists,
 * NULL is returned.
 * otherwise, the pointer to the new variable entry is returned
 *   with the specified name filled.
 */
struct var_entry * add_var_entry(const char *);

/**
 * search a variable entry in the WHOLE table.
 * if not found, NULL is returned.
 * otherwise, the entry is returned.
 */
struct var_entry * search_var_entry(const char *);

struct param_entry {
	struct list_head list_ptr;
	struct var_entry * param_ptr;
};

struct func_entry {
	char name[NAME_LEN];				// name of the function
	unsigned short return_type_flag;	// basic, struct or array?
	union var_type return_type;
	unsigned int defined_at;			// NOTE: declared once defined

	struct list_head param_list;		// NOTE: params are stored in var table, not owned by func

	struct list_head list_ptr;			// points to next function
};

/**
 * initialize the function table.
 * should be called when analyzing a new .cmm code file.
 */
void init_func_table();

/**
 * clear the function table.
 * should be called when the current code file has been analyzed.
 */
void clear_func_table();

/**
 * add a function entry in the function table.
 * if the name conflict exists, NULL is returned.
 * otherwise, the pointer to the function entry is returned, with
 *   parameter list head initialized.
 */
struct func_entry * add_func_entry(const char *);

/**
 * search for the function with given name.
 * if exists, corresponding function entry is returned.
 * otherwise, NULL is returned.
 */
struct func_entry * search_func_entry(const char *);

/**
 * add a parameter in the function.
 * parameter is placed to the TAIL,
 * and the entry ptr is returned.
 */
struct var_entry * add_func_param(struct func_entry *, struct var_entry *);

/**
 * search for a parameter in the designated function.
 * if the parameter does not exist, NULL is returned.
 * otherwise, the variable entry pointer is returned.
 */
struct var_entry * search_func_param(struct func_entry *, const char *);


/**************************** STRUCT TABLE ***************************/

struct member_entry {
	struct list_head list_ptr;
	struct var_entry * member_ptr;
	unsigned short offset;
};

struct struct_entry {
	char name[NAME_LEN];				// name of the struct
	unsigned int declared_at;			// where it is FIRST declared
	unsigned int defined_at;			// where it is defined
	unsigned int is_defined;			// WHETHER it is defined
	unsigned short size;

	struct list_head member_list;		// list head of members
	// NOTE: members are stored in variable table, not owned by the struct.

	struct list_head list_ptr;			// points to next struct
};

/**
 * initialize the struct table.
 * used when analysis begins.
 */
void init_struct_table();

/**
 * clear the struct table.
 * used when analysis for the .cmm code file is about to end.
 */
void clear_struct_table();

/**
 * add a new struct entry.
 * all struct names cannot conflict,
 * even if defined nested inside another struct.
 * if conflict appears, NULL is returned.
 */
struct struct_entry * add_struct_entry(const char *);

/**
 * search for a struct entry.
 * if no struct is found, NULL is returned.
 */
struct struct_entry * search_struct_entry(const char *);

/**
 * add a member to a struct entry.
 * if conflict happens anywhere, NULL is returned.
 * otherwise, the new member will be CREATED at the TAIL of the list and returned.
 */
struct var_entry * add_struct_member(struct struct_entry *, struct var_entry *);

/**
 * search for a member in the designated struct
 * if no such member, NULL is returned, otherwise the member entry is returned.
 */
struct var_entry * search_struct_member(struct struct_entry *, const char *);


/**
 * clear all members of the designated struct entry.
 * used by clear_struct_table commonly.
 *
int clear_struct_members(struct struct_entry *);
*/
#endif // __var_H__
