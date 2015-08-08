#include "memory_table.h"

#define SYMBOL_MAX 4096

extern struct intercode * intercodes[INSTR_COUNT];

extern int streq(const char *, const char *);

const char * symbol_pool[SYMBOL_MAX] = {NULL};
const char * param_pool[SYMBOL_MAX] = {NULL};
struct intercode * dec_pool[SYMBOL_MAX] = {NULL};
int symbol_count, dec_count, param_count;


int add_symbol_str(const char *str) {
	int i = 0;
	for (i = 0; i < dec_count; ++i) {
		if ( streq(dec_pool[i] -> dec.variable, str) )
			return 0;
	}
	for (i = 0; i < param_count; ++i) {
		if ( streq(param_pool[i], str) )
			return 0;
	}
	for (i = 0; i < symbol_count; ++i) {
		if ( streq(symbol_pool[i], str) )
			return 0;
	}
	symbol_pool[i] = str;
	return 1;
}

int add_symbol(const struct operand * opr) {
	const char * str = NULL;
	if (opr -> type == OPR_VARIABLE) str = opr -> opr_variable;
	else if (opr -> type == OPR_CONSTANT) return 0;
	else if (opr -> type == OPR_GETADDR) str = opr -> addr_variable;
	else if (opr -> type == OPR_DEREFERENCE) str = opr -> deref_address;
	return add_symbol_str(str);
}

struct memory_table * 
make_memory_table(int fstart, int fend) {
	memset(symbol_pool, 0, sizeof(symbol_pool));
	memset(dec_pool, 0, sizeof(dec_pool));
	memset(param_pool, 0, sizeof(param_pool));
	symbol_count = dec_count = param_count = 0;
	int i = fstart;
	for (; i < fend; ++i) {
		struct intercode * ic = intercodes[i];
		switch (ic -> type) {
			case IR_LABEL:		// no symbols in these instructions
			case IR_FUNCTION:
			case IR_GOTO:
				break;
			case IR_ASSIGN:
				symbol_count += add_symbol(&ic -> assign.target);
				symbol_count += add_symbol(&ic -> assign.rhs);
				break;
			case IR_ARITH:
				symbol_count += add_symbol(&ic -> arith_operation.target);
				symbol_count += add_symbol(&ic -> arith_operation.operand1);
				symbol_count += add_symbol(&ic -> arith_operation.operand2);
				break;
			case IR_IFGOTO:
				symbol_count += add_symbol(&ic -> if_goto.operand1);
				symbol_count += add_symbol(&ic -> if_goto.operand2);
				break;
			case IR_RETURN:
				symbol_count += add_symbol(&ic -> return_target);
				break;
			case IR_DEC:
				dec_pool[dec_count] = ic;
				dec_count ++;
				break;
			case IR_ARG:
				symbol_count += add_symbol(&ic -> argument);
				break;
			case IR_CALL:
				symbol_count += add_symbol(&ic -> call.target);
				break;
			case IR_PARAM:
				// parameter does not conflict with symbols or DECs
				// left most parameter is stored at the nearest place to $fp
				param_pool[param_count] = ic -> parameter;
				param_count++;
				
				break;
			case IR_READ:
				symbol_count += add_symbol(&ic -> read_name);
				break;
			case IR_WRITE:
				symbol_count += add_symbol(&ic -> write_name);
				break;
		}
	}
	assert( symbol_pool[symbol_count - 1] );
	assert( !symbol_pool[symbol_count] );

	struct memory_table * ret = (struct memory_table *) malloc (sizeof(struct memory_table));
	ret -> num_entry = symbol_count + dec_count;
	ret -> symbols = (const char **) malloc (sizeof(char *) * ret -> num_entry);
	ret -> sizes = (int *) malloc( sizeof(int) * ret -> num_entry );
	ret -> offsets = (int *) malloc( sizeof(int) * ret -> num_entry );

	int mti = 0;
	for (i = 0; i < dec_count; ++i) {
		ret -> symbols[mti] = dec_pool[i] -> dec.variable;
		ret -> sizes[mti] = dec_pool[i] -> dec.size;
		mti++;
	}
	for(i = 0; i < symbol_count; ++i) {
		int dup = 0, di = 0;
		ret -> symbols[mti] = symbol_pool[i];
		ret -> sizes[mti] = 4;
		mti++;
	}
	for (i = 0; i < ret -> num_entry; ++i) {
		if (i == 0)
			ret -> offsets[i] = ret -> sizes[i];
		else
			ret -> offsets[i] = ret -> sizes[i] + ret -> offsets[i-1];
	}

	ret -> num_params = param_count;
	ret -> parameters = (const char **) malloc (sizeof(char *) * ret -> num_params);
	ret -> param_offsets = (int *) malloc( sizeof(int) * ret -> num_params);

	for (i = 0; i < param_count; ++i) {
		ret -> parameters[i] = param_pool[i];
		ret -> param_offsets[i] = -4 * i - 8; // TODO: or i+1?
	}
	return ret;
}
			
int get_size(struct memory_table * mt, const char * symbol) {
	int i;
	for (i = 0; i < mt -> num_entry; ++i) {
		if ( mt -> symbols[i] == NULL ) continue;
		if ( streq(mt -> symbols[i], symbol) )
			return mt -> sizes[i];
	}
	for (i = 0; i < mt -> num_params; ++i) {
		if ( mt -> parameters[i] == NULL ) continue;
		if ( streq(mt -> parameters[i], symbol) )
			return 4;
	}
	return -1;
}

int get_position(struct memory_table * mt, const char * symbol) {
	int i;
	for (i = 0; i < mt -> num_entry; ++i) {
		if ( mt -> symbols[i] == NULL ) continue;
		if ( streq(mt -> symbols[i], symbol) )
			return mt -> offsets[i];
	}
	for (i = 0; i < mt -> num_params; ++i) {
		if ( mt -> parameters[i] == NULL ) continue;
		if ( streq(mt -> parameters[i], symbol) )
			return mt -> param_offsets[i];
	}
	return -1;
}
				


