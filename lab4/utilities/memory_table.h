#ifndef __MEMORY_TABLE_H__
#define __MEMORY_TABLE_H__

#include <string.h>
#include "../intercode.h"

struct memory_table {
	int num_entry;
	const char ** symbols;
	int * sizes;
	int * offsets;

	int num_params;
	const char ** parameters;
	int * param_offsets;
};

struct memory_table *
make_memory_table(int fstart, int fend);

int
get_size(struct memory_table * mt, const char *symbol);

int
get_position(struct memory_table * mt, const char * symbol);


#endif // __MEMORY_TABLE_H__
