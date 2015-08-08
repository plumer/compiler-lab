#include "symbol.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#define MAXDEPTH 32

struct list_head var_table;
struct list_head * curr_var_table;
static int __var_n;

void clear_array(struct array **a) {
	if ( (*a) -> dimension != 1)
		clear_array( &( (*a) -> elem.array_type) );
	free(*a);
	*a = NULL;
	return;
}

void init_var_table() {
	INIT_LIST_HEAD(&var_table);
	__var_n = 0;
}

void clear_var_table() {
	struct list_head *p, *tmp;
	struct var_entry *ve;
	list_for_each_safe(p, tmp, &var_table) {
		ve = list_entry(p, struct var_entry, list_ptr);
		list_del(p);
		if (ve -> type_flag == TYPE_ARRAY)
			clear_array( &(ve -> t.array_type) );
		free(ve);
	}
	__var_n = 0;
	assert(list_empty(&var_table));
}

struct var_entry *
add_var_entry(const char * name) {
	//printf("[ %s ], name = \'%s\'\n", __FUNCTION__, name);
	if (search_var_entry(name)) return NULL;
	if (search_struct_entry(name)) return NULL;
	struct var_entry * new_ve = (struct var_entry *)malloc(sizeof(struct var_entry));
	if (new_ve == NULL) return NULL;
	memset(new_ve, 0, sizeof(struct var_entry));
	strncpy(new_ve -> name, name, NAME_LEN - 1);
	list_add_tail(&(new_ve -> list_ptr), &var_table);
	__var_n++;
	return new_ve;
}

struct var_entry *
search_var_entry(const char * name) {
	struct list_head * p;
	struct var_entry * ve;
	list_for_each(p, &var_table) {
		ve = list_entry(p, struct var_entry, list_ptr);
		if (strlen(ve -> name) != strlen(name)) continue;
		if (strncmp(ve -> name, name, strlen(name)) == 0) return ve;
	}
	return NULL;
}


// ************************ FUNCTION AREA ******************8

struct list_head func_table;
static int __func_n;

void init_func_table() {
	INIT_LIST_HEAD(&func_table);
	__func_n = 0;
}

void clear_func_table() {
	struct list_head *p, *tmp;
	struct func_entry *fe;
	list_for_each_safe(p, tmp, &func_table) {
		fe = list_entry(p, struct func_entry, list_ptr);
		struct list_head *inner_p, *inner_tmp;
		struct param_entry * inner_pe;
		list_for_each_safe(inner_p, inner_tmp, &(fe -> param_list) ) {
			inner_pe = list_entry(inner_p, struct param_entry, list_ptr);
			list_del(inner_p);
			free(inner_pe);
		}
		list_del(p);
		free(fe);
	}
	__func_n = 0;
	assert( list_empty( &func_table ) );
}

struct func_entry *
add_func_entry(const char *name) {
	if (search_func_entry(name)) return NULL;
	struct func_entry *new_fe = (struct func_entry *)malloc( sizeof(struct func_entry) );
	if (new_fe == NULL) return NULL;
	memset(new_fe, 0, sizeof(struct func_entry));
	strncpy(new_fe -> name, name, NAME_LEN - 1);
	list_add_tail( &(new_fe -> list_ptr), &func_table );
	INIT_LIST_HEAD( &(new_fe -> param_list) );
	__func_n ++;
	return new_fe;
}

struct func_entry *
search_func_entry(const char * name) {
	struct list_head *p;
	struct func_entry *fe;
	
	list_for_each(p, &func_table) {
		fe = list_entry(p, struct func_entry, list_ptr);
		if (strlen(fe -> name) != strlen(name)) continue;
		// one big bug:						 should not ^ return NULL;
		if ( strncmp(fe -> name, name, strlen(name)) ==0) return fe;
	}
	return NULL;
}

struct var_entry * add_func_param(struct func_entry *fe, struct var_entry *ve) {
	assert(fe);
	assert(ve);
	struct param_entry * pe = (struct param_entry *)malloc( sizeof(struct param_entry) );
	pe -> param_ptr = ve;
	list_add_tail( &(pe -> list_ptr), &(fe -> param_list) );
	return ve;
}

struct var_entry * search_func_param(struct func_entry *fe, const char *name) {
	assert(fe);
	assert(name);

	struct list_head *p;
	struct var_entry *pe;
	list_for_each( p, &(fe -> param_list) ) {
		pe = list_entry(p, struct param_entry, list_ptr) -> param_ptr;
		if ( strlen(pe -> name) != strlen(name) ) continue;
		if ( strncmp(pe -> name, name, strlen(name)) == 0) return pe;
	}
	if (search_var_entry(name)) panic();
	return NULL;
}
// ************************ STRUCT AREA  **********************

struct list_head struct_table;
static int __struct_n;

void init_struct_table() {
	INIT_LIST_HEAD(&struct_table);
	__struct_n = 0;
}

void clear_struct_table() {
	struct list_head *p, *tmp;
	struct struct_entry * se;
	list_for_each_safe(p, tmp, &struct_table) {
		se = list_entry(p, struct struct_entry, list_ptr);
		
		struct list_head * inner_p, *n;
		struct member_entry * inner_me;
		list_for_each_safe( inner_p, n, &(se -> member_list) ) {
			inner_me = list_entry(inner_p, struct member_entry, list_ptr);
			list_del(inner_p);
			free(inner_me);
		}

		list_del(p);
		free(se);
	}
	__struct_n = 0;
}

struct struct_entry *
add_struct_entry(const char *name) {
	if (search_struct_entry(name)) return NULL;
	if (search_var_entry(name)) return NULL;
	struct struct_entry *se = (struct struct_entry *)malloc( sizeof(struct struct_entry) );
	memset(se, 0, sizeof(struct struct_entry));
	strncpy(se -> name, name, NAME_LEN - 1);
	list_add_tail( &(se -> list_ptr), &struct_table);
	INIT_LIST_HEAD( &(se -> member_list) );
	__struct_n ++;
	return se;
}

struct struct_entry *
search_struct_entry(const char * name) {
	struct list_head *p;
	struct struct_entry *se;
	list_for_each(p, &struct_table) {
		se = list_entry(p, struct struct_entry, list_ptr);
		if ( strlen(se -> name) != strlen(name) ) continue;
		if ( strncmp(se -> name, name, strlen(name)) ==0 ) return se; 
	}
	return NULL;
}

struct var_entry *
add_struct_member(struct struct_entry *se, struct var_entry *ve) {
	if (ve == NULL) { panic(); return NULL; }
	struct member_entry * me = (struct member_entry *)malloc( sizeof(struct member_entry) );
	me -> member_ptr = ve;
	list_add_tail( &(me -> list_ptr), &(se -> member_list) );
	return ve;
}

struct var_entry *
search_struct_member(struct struct_entry *se, const char *name) {
	struct list_head *p;
	struct var_entry *me;
	//panic();
	list_for_each(p, &(se -> member_list) ) {
		me = list_entry(p, struct member_entry, list_ptr) -> member_ptr;
		//printf("me -> name = \'%s\', name = \'%s\'\n",me -> name, name);
		if ( strlen(me -> name) != strlen(name) ) continue;
		if ( strncmp(me -> name, name, strlen(name) ) == 0 ) return me;
	}
	if (search_var_entry(name)) panic();
	return NULL;
}
