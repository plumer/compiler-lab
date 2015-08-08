#ifndef __INTERCODE_H__
#define __INTERCODE_H__

#include <assert.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "syntax.tab.h"
#include "semantic.h"
#include "utilities/tree.h"
#include "utilities/symbol.h"

#define INSTR_COUNT 4096
#define INSTR_LEN	128

void init_label_flags();


char * new_param_name();
char * new_arg_name();
char * new_temp_name();
char * new_label_name();
char * new_name(const char *, ...);

char * get_op_str(struct tree_node *);

void preprocess();			// adds write and read func
void translate(struct tree_node * );
void translate_bool(struct tree_node *);
void translate_subscript(struct tree_node *);
void translate_args(struct tree_node *);
void output_code(const char *, ...);
void flush_code();

int get_next_instr();

void backpatch(struct list_head * head, int instr);
void mergelist(struct list_head * target, 
		struct list_head * l1, struct list_head * l2);
void makelist(struct list_head * target, int instr);
void movelist(struct list_head * target, struct list_head * source);
void printlist(struct list_head * head);
char * outputfile;

int is_constant(const char *);
int operate(int l, struct tree_node *, int r);
char * optimize_binop(struct tree_node *ptr);

extern char * instructions[INSTR_COUNT] ;
extern char label_flags[INSTR_COUNT];
extern int instruction_count;

struct operand {
	enum {
		OPR_VARIABLE,
		OPR_CONSTANT,
		OPR_GETADDR,
		OPR_DEREFERENCE
	} type ;

	union {
		const char * opr_variable;
		int opr_value;
		const char * addr_variable;
		const char * deref_address;
	} op;
#define opr_variable	op.opr_variable
#define opr_value		op.opr_value
#define addr_variable	op.addr_variable
#define deref_address	op.deref_address
};



enum {
	IR_LABEL, IR_FUNCTION, 
	IR_ASSIGN, IR_ARITH, 
	IR_GOTO,	IR_IFGOTO,
	IR_RETURN,
	IR_DEC,
	IR_ARG,		IR_CALL,	IR_PARAM,
	IR_READ,	IR_WRITE
};

struct intercode {
	int type;
	union {
		const char * label_name;
		const char * function_name;
		struct {
			struct operand target;
			struct operand rhs;
		} assign;
		struct {
			struct operand target;
			struct operand operand1;
			const char * arith_operator;
			struct operand operand2;
		} arith_operation;
		const char * goto_target;
		struct {
			struct operand operand1;
			const char * bool_operator;
			struct operand operand2;
			const char * if_goto_target;
		} if_goto;
		struct operand return_target;
		struct {
			const char * variable;
			int size;
		} dec;
		struct operand argument;
		struct {
			struct operand target;
			const char * call_function_name;
		} call;
		const char * parameter;
		struct operand read_name;
		struct operand write_name;
	} content;
#define label_name		content.label_name
#define function_name	content.function_name
#define assign			content.assign
#define arith_operation	content.arith_operation
#define goto_target		content.goto_target
#define if_goto			content.if_goto
#define return_target	content.return_target
#define dec				content.dec
#define argument		content.argument
#define call			content.call
#define parameter		content.parameter
#define read_name		content.read_name
#define write_name		content.write_name
};


#endif // __INTERCODE_H__
