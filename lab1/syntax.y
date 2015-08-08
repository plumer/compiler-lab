%{
	#include <stdio.h>
	#include <stdarg.h>
	#include "tree.h"
	struct treeNode *root;
	int error = 0;
	int lineno = 0;

	void parserror(const char *, ...);
	void lexerror(const char *, ...);
%}

%union {
	int type_int;
	struct treeNode * type_node;
};

//%type <type_node> Unexpected
%token <type_node> INT FLOAT
%token <type_node> ID
%token <type_node> SEMI COMMA
%token <type_node> ASSIGNOP
%token <type_node> RELOP
%token <type_node> PLUS MINUS STAR DIV AND OR 
%token <type_node> DOT
%token <type_node> NOT
%token <type_node> LP RP LB RB LC RC
%token <type_node> STRUCT RETURN IF ELSE WHILE
%token <type_node> UNDEF
%token <type_node> TYPE
%type <type_node> Program ExtDefList ExtDef ExtDecList
%type <type_node> Specifier StructSpecifier OptTag Tag
%type <type_node> VarDec FunDec VarList ParamDec
%type <type_node> CompSt StmtList Stmt
%type <type_node> DefList Def DecList Dec
%type <type_node> Exp Args

%right ASSIGNOP
%left OR
%left AND
%left RELOP
%left PLUS MINUS 
%left STAR DIV
%right NOT 
%left DOT LB RB LP RP

%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE

%%

Program : ExtDefList		{
		$$ = createNode("Program", $1 -> line);
		addChild($$, $1, 0);
		root = $$;
	}
	;
ExtDefList : ExtDef ExtDefList {
		$$ = createNode("ExtDefList", $1 -> line);
		addChild($$, $1, 0);
		addChild($$, $2, 1);
		$$ -> flags |= FLAG_MULTI;
		$$ -> flags |= 2;
	} | {
		$$ = createEndNode("No more ExtDefs");
	}
	;
ExtDef : Specifier ExtDecList SEMI {
		$$ = createNode("ExtDef: global variables", $1 -> line);
		addChild($$, $1, 0); addChild($$, $2, 1); addChild($$, $3, 2);
	} | Specifier SEMI {
		$$ = createNode("ExtDef: maybe structs", $1 -> line);
		addChild($$, $1, 0); addChild($$, $2, 1);
	} | Specifier FunDec CompSt {
		$$ = createNode("ExtDef: a function", $1 -> line);
		addChild($$, $1, 0); addChild($$, $2, 1); addChild($$, $3, 2);
	} | error SEMI {
		parserror("Error exterior definition\n");
		$$ = createNode("ExtDef", lineno);
	} | Specifier {
		parserror("Maybe a semicolon is missing\n");
		$$ = createNode("ExtDef", lineno);
	}
	;
ExtDecList : VarDec {
		$$ = createNode("ExtDecList", $1 -> line);
		addChild($$, $1, 0);
		$$ -> flags |= FLAG_MULTI;
		$$ -> flags |= 1;
	} | VarDec COMMA ExtDecList {
		$$ = createNode("ExtDecList", $1 -> line);
		addChild($$, $1, 0); addChild($$, $2, 1); addChild($$, $3, 2);
		$$ -> flags |= FLAG_MULTI;
		$$ -> flags |= 3;
	}
	;

Specifier : TYPE {
		$$ = createNode("Specifier: basic type", $1 -> line);
		addChild($$, $1, 0);
	} | StructSpecifier {
		$$ = createNode("Specifier: struct type", $1 -> line);
		addChild($$, $1, 0);
	}
	;
StructSpecifier : STRUCT OptTag LC DefList RC {
		$$ = createNode("Struct Specifier: definition", $1 -> line);
		addChild($$, $1, 0); addChild($$, $2, 1); addChild($$, $3, 2);
		addChild($$, $4, 3); addChild($$, $5, 4);
	} | STRUCT OptTag DefList RC{
		parserror("Missing '{' in struct definition\n");
	} | STRUCT Tag {
		$$ = createNode("Struct Specifier: tag only", $1 -> line);
		addChild($$, $1, 0); addChild($$, $2, 1);
	}
	;
OptTag : ID {
		$$ = createNode("OptTag", $1 -> line);
		addChild($$, $1, 0);
	} | {
		$$ = createEndNode("No more OptTags");
	}
	;
Tag : ID {
		$$ = createNode("Tag", $1 -> line);
		addChild($$, $1, 0);
	}
	;

// Declarators

VarDec : ID {
		$$ = createNode("VarDec: an identifier", $1 -> line);
		addChild($$, $1, 0);
	} | VarDec LB INT RB {
		$$ = createNode("VarDec: an array", $1 -> line);
		addChild($$, $1, 0); addChild($$, $2, 1); addChild($$, $3, 2); addChild($$, $4, 3);
	} | VarDec LB error RB {
		parserror("Integer only allowed inside brackets\n");
	}
	;
FunDec : ID LP VarList RP {
		$$ = createNode("FunDec: with parameters", $1 -> line);
		addChild($$, $1, 0); addChild($$, $2, 1); addChild($$, $3, 2); addChild($$, $4, 3);
	} | ID LP RP {
		$$ = createNode("FunDec: no parameters", $1 -> line);
		addChild($$, $1, 0); addChild($$, $2, 1); addChild($$, $3, 2);
	} | ID error RP {
		parserror("Invalid function header, maybe a '(' is missing\n");
	} | ID LP error LC {
		parserror("Invalid function header, maybe a ')' is missing\n");
	}
	;
VarList : ParamDec COMMA VarList {
		$$ = createNode("Multi Varlist", $1 -> line);
		$$ -> flags |= FLAG_MULTI;
		$$ -> flags |= 3;
		addChild($$, $1, 0); addChild($$, $2, 1); addChild($$, $3, 2);
	} | ParamDec {
		$$ = createNode("VarList", $1 -> line);
		addChild($$, $1, 0);
	}
	;
ParamDec : Specifier VarDec {
		$$ = createNode("ParamDec", $1 -> line);
		addChild($$, $1, 0); addChild($$, $2, 1);
	}
	;

// Statements

CompSt : LC DefList StmtList RC {
		$$ = createNode("CompSt", $1 -> line);
		addChild($$, $1, 0); addChild($$, $2, 1); addChild($$, $3, 2); addChild($$, $4, 3);
	}
	;
StmtList : Stmt StmtList {
		$$ = createNode("StmtList", $1 -> line);
		addChild($$, $1, 0); addChild($$, $2, 1);
		$$ -> flags |= FLAG_MULTI;
		$$ -> flags |= 2;
	} | {
		$$ = createEndNode("no more statements");
	}
	;
Stmt : Exp SEMI {
		$$ = createNode("Stmt: Expression", $1 -> line);
		addChild($$, $1, 0); addChild($$, $2, 1);
	} | CompSt {
		$$ = createNode("Stmt: Composed stmts", $1 -> line);
		addChild($$, $1, 0);
	} | RETURN Exp SEMI	 {
		$$ = createNode("Stmt: Return", $1 -> line);
		addChild($$, $1, 0); addChild($$, $2, 1); addChild($$, $3, 2);
	} | IF LP Exp RP Stmt %prec LOWER_THAN_ELSE {
		$$ = createNode("Stmt: IF clause", $1 -> line);
		addChild($$, $1, 0); addChild($$, $2, 1); addChild($$, $3, 2); addChild($$, $4, 3); addChild($$, $5, 4);
	} | IF error Stmt %prec LOWER_THAN_ELSE {
		parserror("invalid condition expression in IF clause\n");
	} | IF LP Exp RP Stmt ELSE Stmt {
		$$ = createNode("Stmt IF-ELSE clause", $1 -> line);
		addChild($$, $1, 0); addChild($$, $2, 1); addChild($$, $3, 2); addChild($$, $4, 3); addChild($$, $5, 4); addChild($$, $6, 5); addChild($$, $7, 6);
	} | IF error Stmt ELSE Stmt {
		parserror("Invalid condition expression in IF-ELSE clause\n");
	} | WHILE LP Exp RP Stmt {
		$$ = createNode("Stmt: WHILE clause", $1 -> line);
		addChild($$, $1, 0); addChild($$, $2, 1); addChild($$, $3, 2); addChild($$, $4, 3); addChild($$, $5, 4);
	} | WHILE error Stmt{
		parserror("invalid condition expression in WHILE clause\n");
	} | error SEMI {
		parserror("Unrecognized statement\n");
	} | error RC {
// 该产生式行使用了不应该出现在Stmt文法中的语法单元RC，
// 已起到检测「未预料到的右括号」情况，便于定位错误。
// 然而RC会被bison分析程序「吃掉」，会影响后面的分析。
// 如果bison有类似flex一样操作输入缓冲区和分析栈的函数的话，这个问题应该可以更好地解决。
		parserror("missing ';' before right curly brace\n");
	}
	;

// Local Definitions

DefList : Def DefList {
		$$ = createNode("DefList", $1 -> line);
		addChild($$, $1, 0); addChild($$, $2, 1);
		$$ -> flags |= FLAG_MULTI;
		$$ -> flags |= 2;
	} | {
		$$ = createEndNode("No more Defs");
	}
	;
Def : Specifier DecList SEMI {
		$$ = createNode("Def", $1 -> line);
		addChild($$, $1, 0); addChild($$, $2, 1); addChild($$, $3, 2);
	} | Specifier error SEMI {
		parserror("invalid variable definition\n");
		}
	;
DecList : Dec {
		$$ = createNode("DecList", $1 -> line);
		addChild($$, $1, 0);
	} | Dec COMMA DecList {
		$$ = createNode("DecList: Another Dec", $1 -> line);
		addChild($$, $1, 0); addChild($$, $2, 1); addChild($$, $3, 2);
		$$ -> flags |= FLAG_MULTI;
		$$ -> flags |= 3;
	}
	;
Dec : VarDec {
		$$ = createNode("Dec", $1 -> line);
		addChild($$, $1, 0);
	} | VarDec ASSIGNOP Exp {
		$$ = createNode("Dec: with initialization", $1 -> line);
		addChild($$, $1, 0); addChild($$, $2, 1); addChild($$, $3, 2);
	}
	;

// Expressions

Exp : Exp ASSIGNOP Exp {
	//	printf("assignment\n");
		$$ = createNode("Exp: assignment", $1 -> line);
		addChild($$, $1, 0); addChild($$, $2, 1); addChild($$, $3, 2);
	} | Exp AND Exp {
		$$ = createNode("Exp: bit-wise and", $1 -> line);
		addChild($$, $1, 0); addChild($$, $2, 1); addChild($$, $3, 2);
	} | Exp OR Exp {
		$$ = createNode("Exp: bit-wise or", $1 -> line);
		addChild($$, $1, 0); addChild($$, $2, 1); addChild($$, $3, 2);
	} | Exp RELOP Exp {
		$$ = createNode("Exp: relational", $1 -> line);
		addChild($$, $1, 0); addChild($$, $2, 1); addChild($$, $3, 2);
	} | Exp PLUS Exp	{
		$$ = createNode("Exp: addition", $1 -> line);
		addChild($$, $1, 0); addChild($$, $2, 1); addChild($$, $3, 2);
	} | Exp MINUS Exp {
		$$ = createNode("Exp: subtraction", $1 -> line);
		addChild($$, $1, 0); addChild($$, $2, 1); addChild($$, $3, 2);
	} | Exp STAR Exp {
		$$ = createNode("Exp: multiplication", $1 -> line);
		addChild($$, $1, 0); addChild($$, $2, 1); addChild($$, $3, 2);
	} | Exp DIV Exp {
		$$ = createNode("Exp: division", $1 -> line);
		addChild($$, $1, 0); addChild($$, $2, 1); addChild($$, $3, 2);
	} | LP Exp RP {
		$$ = createNode("Exp: in parantheses", $1 -> line);
		addChild($$, $1, 0); addChild($$, $2, 1); addChild($$, $3, 2);
	} | MINUS Exp {
		$$ = createNode("Exp: negation", $1 -> line);
		addChild($$, $1, 0); addChild($$, $2, 1);
	} | NOT Exp {
		$$ = createNode("Exp: logical not", $1 -> line);
		addChild($$, $1, 0); addChild($$, $2, 1);
	} | ID LP Args RP {
		$$ = createNode("Exp: function call with params", $1 -> line);
		addChild($$, $1, 0); addChild($$, $2, 1); addChild($$, $3, 2); addChild($$, $4, 3);
	} | ID LP RP {
		$$ = createNode("Exp: function call", $1 -> line);
		addChild($$, $1, 0); addChild($$, $2, 1); addChild($$, $3, 2);
	} | Exp LB Exp RB {
		$$ = createNode("Exp: array subscript", $1 -> line);
		addChild($$, $1, 0); addChild($$, $2, 1); addChild($$, $3, 2); addChild($$, $4, 3);
	} | Exp LB error RB {
		parserror("Invalid expression between brackets\n");
	} | Exp DOT ID {
		$$ = createNode("Exp: member designation", $1 -> line);
		addChild($$, $1, 0); addChild($$, $2, 1); addChild($$, $3, 2);
	} | ID		{
		$$ = createNode("Exp: identifier", $1 -> line);
		addChild($$, $1, 0);
	} | INT		{
		$$ = createNode("Exp: integer", $1 -> line);
		addChild($$, $1, 0);
	} | FLOAT		{
		$$ = createNode("Exp: float", $1 -> line);
		addChild($$, $1, 0);
	}
	;
Args : Exp COMMA Args {
		$$ = createNode("Args: another arg", $1 -> line);
		addChild($$, $1, 0); addChild($$, $2, 1); addChild($$, $3, 2);
		$$ -> flags |= FLAG_MULTI;
		$$ -> flags |= 3;
	} | Exp {
		$$ = createNode("Arg", $1 -> line);
		addChild($$, $1, 0);
	}
	;

%%

#include "lex.yy.c"
#include <assert.h>
#include <stdio.h>

int main(int argc, char ** argv) {
	FILE * f;
	if (argc == 1) {
		f = fopen("../Test/test1.cmm", "r");
	}
	int i = 0;
	for (i = 1; i < argc; ++i) {
		f = fopen(argv[i], "r");
		if (!f) continue;
		printf("Analysis for %s\n------------------\n", argv[i]);
		yyrestart(f);
		lineno = 1;
		error = 0;
		//:yydebug=1;
		yyparse();
		fclose(f);
		if (root && !error) printTree(root);
		putchar('\n');
	}

	return 0;
}
/*
yyerror(char *msg) {
	fprintf(stderr, "error: %s\n", msg);
}
*/
void parserror(const char * fmt, ...) {
	printf("Syntax error at line %d: ", lineno);
	error++;
	va_list arg_ptr;
	va_start(arg_ptr, fmt);
	vprintf(fmt, arg_ptr);
	va_end(arg_ptr);
}

void lexerror(const char *fmt, ...) {
	printf("Lexical error at line %d: ", lineno);
	error++;
	va_list arg_ptr;
	va_start(arg_ptr, fmt);
	vprintf(fmt, arg_ptr);
	va_end(arg_ptr);
}
