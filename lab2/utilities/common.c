#include "common.h"
#include <stdarg.h>
#include <stdio.h>

int error;
int lineno = 0;
struct tree_node *root;
const char * current_file = NULL;

void reseterror() {
	error = 0;
}

void resetlineno() {
	lineno = 0;
}

void lexerror(const char *fmt, ...) {
	// printf("Lexical error at line %d, file %s: ", lineno, current_file);
	printf("Lexical error at line %d: ", lineno);
	error++;
	va_list arg_ptr;
	va_start(arg_ptr, fmt);
	vprintf(fmt, arg_ptr);
	va_end(arg_ptr);
}
void synerror(const char * fmt, ...) {
	//printf("Syntax error at line %d, file %s: ", lineno, current_file);
	printf("Syntax error at line %d: ", lineno);
	error++;
	va_list arg_ptr;
	va_start(arg_ptr, fmt);
	vprintf(fmt, arg_ptr);
	va_end(arg_ptr);
}

void semerrorpos(int type, int line) {
	//printf("Semantic error at line %d, file %s: ", line, current_file);
	printf("Semantic error %d at line %d: ", type, line);
}



const char * unit_str[] = {
	"Undef",
	"Integer", "Float", "Identifier",
	"Semicolon", "Comma", "Assignment Operator",
	"Relation Operator", "Plus Operator", "Minus Operator",
	"Star", "Div", "And",
	"Or", "Dot", "Not",
	"Type", "Parentheses", "Bracket",
	"Curly Brace", "Keyword Struct", "Keyword Return",
	"Keyword IF", "Keyword ELSE", "Keyword WHILE",
	"Undefined TOKEN",

	"Program",
	"ExtDefList", "ExtDef", "ExtDecList",
	"Specifier", "Struct Specifier", "OptTag",
	"Tag",
	"VarDec", "FunDec", "VarList",
	"ParamDec", 
	"Composed Statement", "Statement List", "Statement",
	"DefList", "Def", "DecList",
	"Dec",
	"Expression", "Arguments",
	"End of List"
};


const char * type_str[] = {
	"error",
	"integer", 		"float",
	"basic",		"array",		"struct"
};

const char * compare_str[] = {
	"good",
	"kind mismatch",
	"basic type mismatch",
	"array dimension mismatch",
	"struct mismatch",
	"unknown"
};

const char * unit_to_string (unit_t u) {
	return unit_str[u];
}
