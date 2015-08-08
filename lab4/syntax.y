%{
	#include <stdio.h>
	#include <stdarg.h>
	#include <assert.h>
	#include "utilities/tree.h"
%}

%union {
	struct tree_node * type_node;
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
		$$ = create_node(NODE_PROGRAM, $1 -> line);
		add_child($$, $1, 0);
		root = $$;
	}
	;
ExtDefList : ExtDef ExtDefList {
		$$ = create_node(NODE_EXTDEFLIST, $1 -> line);
		add_child($$, $1, 0);
		add_child($$, $2, 1);
		$$ -> flags = FLAG_EXTDEFLIST_MORE;
	} | {
		$$ = create_end_node(NODE_LIST_END);
	}
	;
ExtDef : Specifier ExtDecList SEMI {
		$$ = create_node(NODE_EXTDEF, $1 -> line);
		$$ -> flags = FLAG_EXTDEF_VARIABLES;
		add_child($$, $1, 0); add_child($$, $2, 1); add_child($$, $3, 2);
	} | Specifier SEMI {
		$$ = create_node(NODE_EXTDEF, $1 -> line);
		$$ -> flags = FLAG_EXTDEF_TYPE;
		add_child($$, $1, 0); add_child($$, $2, 1);
	} | Specifier FunDec CompSt {
		$$ = create_node(NODE_EXTDEF, $1 -> line);
		$$ -> flags = FLAG_EXTDEF_FUNCTION;
		add_child($$, $1, 0); add_child($$, $2, 1); add_child($$, $3, 2);
	} | Specifier FunDec SEMI {
		synerror("Function declaration not allowed\n");
		$$ = create_node(NODE_EXTDEF, lineno);
	} | error SEMI {
		synerror("Error exterior definition\n");
		$$ = create_node(NODE_EXTDEF, lineno);
		$$ -> flags = FLAG_ERROR;
	} | Specifier {
		synerror("Maybe a semicolon is missing\n");
		$$ = create_node(NODE_EXTDEF, lineno);
		$$ -> flags = FLAG_ERROR;
	}
	;
ExtDecList : VarDec {
		$$ = create_node(NODE_EXTDECLIST, $1 -> line);
		add_child($$, $1, 0);
		$$ -> flags = FLAG_EXTDECLIST_SINGLE;
	} | VarDec COMMA ExtDecList {
		$$ = create_node(NODE_EXTDECLIST, $1 -> line);
		add_child($$, $1, 0); add_child($$, $2, 1); add_child($$, $3, 2);
		$$ -> flags = FLAG_EXTDECLIST_MORE;
	}
	;

Specifier : TYPE {
		$$ = create_node(NODE_SPECIFIER, $1 -> line);
		$$ -> flags = FLAG_SPECIFIER_BASIC;
		add_child($$, $1, 0);
	} | StructSpecifier {
		$$ = create_node(NODE_SPECIFIER, $1 -> line);
		$$ -> flags = FLAG_SPECIFIER_STRUCT;
		add_child($$, $1, 0);
	}
	;
StructSpecifier : STRUCT OptTag LC DefList RC {
		$$ = create_node(NODE_STRUCTSPECIFIER, $1 -> line);
		$$ -> flags = FLAG_STRUCTSPECIFIER_DEF;
		add_child($$, $1, 0); add_child($$, $2, 1); add_child($$, $3, 2);
		add_child($$, $4, 3); add_child($$, $5, 4);
		// printf("struct definition, line = %d\n", $2 -> line);
	} | STRUCT OptTag DefList RC {
		synerror("Missing '{' in struct definition\n");
	} | STRUCT Tag {
		$$ = create_node(NODE_STRUCTSPECIFIER, $1 -> line);
		$$ -> flags = FLAG_STRUCTSPECIFIER_DEC;
		add_child($$, $1, 0); add_child($$, $2, 1);
		// printf("struct declaration, line = %d\n", $2 -> line);
	}
	;
OptTag : ID {
		$$ = create_node(NODE_OPTTAG, $1 -> line);
		$$ -> flags = FLAG_OPTTAG_TAGGED;
		add_child($$, $1, 0);
	} | {
		$$ = create_end_node(NODE_OPTTAG);
		$$ -> flags = FLAG_OPTTAG_EMPTY;
	}
	;
Tag : ID {
		$$ = create_node(NODE_TAG, $1 -> line);
		add_child($$, $1, 0);
	}
	;

// Declarators

VarDec : ID {
		$$ = create_node(NODE_VARDEC, $1 -> line);
		$$ -> flags = FLAG_VARDEC_SINGLE;
		add_child($$, $1, 0);
	} | VarDec LB INT RB {
		$$ = create_node(NODE_VARDEC, $1 -> line);
		$$ -> flags = FLAG_VARDEC_ARRAY;
		add_child($$, $1, 0); add_child($$, $2, 1); add_child($$, $3, 2); add_child($$, $4, 3);
	} | VarDec LB error RB {
		synerror("Integer only allowed inside brackets\n");
	}
	;
FunDec : ID LP VarList RP {
		$$ = create_node(NODE_FUNDEC, $1 -> line);
		$$ -> flags = FLAG_FUNDEC_WITHARGS;
		add_child($$, $1, 0); add_child($$, $2, 1); add_child($$, $3, 2); add_child($$, $4, 3);
	} | ID LP RP {
		$$ = create_node(NODE_FUNDEC, $1 -> line);
		$$ -> flags = FLAG_FUNDEC_NOARGS;
		add_child($$, $1, 0); add_child($$, $2, 1); add_child($$, $3, 2);
	} | ID error RP {
		synerror("Invalid function header, maybe a '(' is missing\n");
	} | ID LP error LC {
		synerror("Invalid function header, maybe a ')' is missing\n");
	}
	;
VarList : ParamDec COMMA VarList {
		$$ = create_node(NODE_VARLIST, $1 -> line);
		$$ -> flags = FLAG_VARLIST_MORE;
		add_child($$, $1, 0); add_child($$, $2, 1); add_child($$, $3, 2);
	} | ParamDec {
		$$ = create_node(NODE_VARLIST, $1 -> line);
		$$ -> flags = FLAG_VARLIST_SINGLE;
		add_child($$, $1, 0);
	}
	;
ParamDec : Specifier VarDec {
		$$ = create_node(NODE_PARAMDEC, $1 -> line);
		add_child($$, $1, 0); add_child($$, $2, 1);
	}
	;

// Statements

CompSt : LC DefList StmtList RC {
		$$ = create_node(NODE_COMPST, $1 -> line);
		add_child($$, $1, 0); add_child($$, $2, 1); add_child($$, $3, 2); add_child($$, $4, 3);
	}
	;
StmtList : Stmt StmtList {
		$$ = create_node(NODE_STMTLIST, $1 -> line);
		add_child($$, $1, 0); add_child($$, $2, 1);
		$$ -> flags = FLAG_STMTLIST_MORE;
	} | {
		$$ = create_end_node(NODE_LIST_END);
	}
	;
Stmt : Exp SEMI {
		$$ = create_node(NODE_STMT, $1 -> line);
		$$ -> flags = FLAG_STMT_EXP;
		add_child($$, $1, 0); add_child($$, $2, 1);
	} | CompSt {
		$$ = create_node(NODE_STMT, $1 -> line);
		$$ -> flags = FLAG_STMT_COMPST;
		add_child($$, $1, 0);
	} | RETURN Exp SEMI	 {
		$$ = create_node(NODE_STMT, $1 -> line);
		$$ -> flags = FLAG_STMT_RETURN;
		add_child($$, $1, 0); add_child($$, $2, 1); add_child($$, $3, 2);
	} | IF LP Exp RP Stmt %prec LOWER_THAN_ELSE {
		$$ = create_node(NODE_STMT, $1 -> line);
		$$ -> flags = FLAG_STMT_IF;
		add_child($$, $1, 0); add_child($$, $2, 1); add_child($$, $3, 2); add_child($$, $4, 3); add_child($$, $5, 4);
	} | IF LP Exp RP Stmt ELSE Stmt {
		$$ = create_node(NODE_STMT, $1 -> line);
		$$ -> flags = FLAG_STMT_IFELSE;
		add_child($$, $1, 0); add_child($$, $2, 1); add_child($$, $3, 2); add_child($$, $4, 3); add_child($$, $5, 4); add_child($$, $6, 5); add_child($$, $7, 6);
	} | WHILE LP Exp RP Stmt {
		$$ = create_node(NODE_STMT, $1 -> line);
		$$ -> flags = FLAG_STMT_WHILE;
		add_child($$, $1, 0); add_child($$, $2, 1); add_child($$, $3, 2); add_child($$, $4, 3); add_child($$, $5, 4);
	} | error SEMI {
		synerror("Unrecognized statement\n");
	} | error RC {
// 该产生式行使用了不应该出现在Stmt文法中的语法单元RC，
// 已起到检测「未预料到的右括号」情况，便于定位错误。
// 然而RC会被bison分析程序「吃掉」，会影响后面的分析。
// 如果bison有类似flex一样操作输入缓冲区和分析栈的函数的话，这个问题应该可以更好地解决。
		synerror("missing ';' before right curly brace\n");
	}
	;

// Local Definitions

DefList : Def DefList {
		$$ = create_node(NODE_DEFLIST, $1 -> line);
		add_child($$, $1, 0); add_child($$, $2, 1);
		$$ -> flags = FLAG_DEFLIST_MORE;
	} | {
		$$ = create_end_node(NODE_LIST_END);
	}
	;
Def : Specifier DecList SEMI {
		$$ = create_node(NODE_DEF, $1 -> line);
		// printf("def, %d\n", $1 -> line);
		add_child($$, $1, 0); add_child($$, $2, 1); add_child($$, $3, 2);
	} 
	;
DecList : Dec {
		$$ = create_node(NODE_DECLIST, $1 -> line);
		$$ -> flags = FLAG_DECLIST_SINGLE;
		add_child($$, $1, 0);
	} | Dec COMMA DecList {
		$$ = create_node(NODE_DECLIST, $1 -> line);
		add_child($$, $1, 0); add_child($$, $2, 1); add_child($$, $3, 2);
		$$ -> flags = FLAG_DECLIST_MORE;
	}
	;
Dec : VarDec {
		$$ = create_node(NODE_DEC, $1 -> line);
		$$ -> flags = FLAG_DEC_NORMAL;
		add_child($$, $1, 0);
	} | VarDec ASSIGNOP Exp {
		$$ = create_node(NODE_DEC, $1 -> line);
		$$ -> flags = FLAG_DEC_INITIALIZED;
		add_child($$, $1, 0); add_child($$, $2, 1); add_child($$, $3, 2);
	}
	;

// Expressions

Exp : Exp ASSIGNOP Exp {
	//	printf("assignment\n");
		$$ = create_node(NODE_EXP, $1 -> line);
		$$ -> flags = FLAG_EXP_ASSIGN;
		add_child($$, $1, 0); add_child($$, $2, 1); add_child($$, $3, 2);
	} | Exp AND Exp {
		$$ = create_node(NODE_EXP, $1 -> line);
		$$ -> flags = FLAG_EXP_BINOP;
		add_child($$, $1, 0); add_child($$, $2, 1); add_child($$, $3, 2);
	} | Exp OR Exp {
		$$ = create_node(NODE_EXP, $1 -> line);
		$$ -> flags = FLAG_EXP_BINOP;
		add_child($$, $1, 0); add_child($$, $2, 1); add_child($$, $3, 2);
	} | Exp RELOP Exp {
		$$ = create_node(NODE_EXP, $1 -> line);
		$$ -> flags = FLAG_EXP_BINOP;
		add_child($$, $1, 0); add_child($$, $2, 1); add_child($$, $3, 2);
	} | Exp PLUS Exp	{
		$$ = create_node(NODE_EXP, $1 -> line);
		$$ -> flags = FLAG_EXP_BINOP;
		add_child($$, $1, 0); add_child($$, $2, 1); add_child($$, $3, 2);
	} | Exp MINUS Exp {
		$$ = create_node(NODE_EXP, $1 -> line);
		$$ -> flags = FLAG_EXP_BINOP;
		add_child($$, $1, 0); add_child($$, $2, 1); add_child($$, $3, 2);
	} | Exp STAR Exp {
		$$ = create_node(NODE_EXP, $1 -> line);
		$$ -> flags = FLAG_EXP_BINOP;
		add_child($$, $1, 0); add_child($$, $2, 1); add_child($$, $3, 2);
	} | Exp DIV Exp {
		$$ = create_node(NODE_EXP, $1 -> line);
		$$ -> flags = FLAG_EXP_BINOP;
		add_child($$, $1, 0); add_child($$, $2, 1); add_child($$, $3, 2);
	} | LP Exp RP {
		$$ = create_node(NODE_EXP, $2 -> line);
		$$ -> flags = FLAG_EXP_PAREN;	
		add_child($$, $1, 0); add_child($$, $2, 1); add_child($$, $3, 2);
	} | MINUS Exp {
		$$ = create_node(NODE_EXP, $1 -> line);
		$$ -> flags = FLAG_EXP_UNOP;
		add_child($$, $1, 0); add_child($$, $2, 1);
	} | NOT Exp {
		$$ = create_node(NODE_EXP, $1 -> line);
		$$ -> flags = FLAG_EXP_UNOP;
		add_child($$, $1, 0); add_child($$, $2, 1);
	} | ID LP Args RP {
		$$ = create_node(NODE_EXP, $1 -> line);
		$$ -> flags = FLAG_EXP_CALL_ARGS;
		add_child($$, $1, 0); add_child($$, $2, 1); add_child($$, $3, 2); add_child($$, $4, 3);
	} | ID LP RP {
		$$ = create_node(NODE_EXP, $1 -> line);
		$$ -> flags = FLAG_EXP_CALL_NOARGS;
		add_child($$, $1, 0); add_child($$, $2, 1); add_child($$, $3, 2);
	} | Exp LB Exp RB {
		$$ = create_node(NODE_EXP, $1 -> line);
		$$ -> flags = FLAG_EXP_SUBSCRIPT;
		add_child($$, $1, 0); add_child($$, $2, 1); add_child($$, $3, 2); add_child($$, $4, 3);
	} | Exp LB error RB {
		synerror("Invalid expression between brackets\n");
	} | Exp DOT ID {
		$$ = create_node(NODE_EXP, $1 -> line);
		$$ -> flags = FLAG_EXP_MEMBER;
		add_child($$, $1, 0); add_child($$, $2, 1); add_child($$, $3, 2);
	} | ID		{
		$$ = create_node(NODE_EXP, $1 -> line);
		$$ -> flags = FLAG_EXP_ID;
		add_child($$, $1, 0);
	} | INT		{
		$$ = create_node(NODE_EXP, $1 -> line);
		$$ -> flags = FLAG_EXP_INT;
		add_child($$, $1, 0);
	} | FLOAT		{
		$$ = create_node(NODE_EXP, $1 -> line);
		$$ -> flags = FLAG_EXP_FLOAT;
		add_child($$, $1, 0);
	}
	;
Args : Exp COMMA Args {
		$$ = create_node(NODE_ARGS, $1 -> line);
		add_child($$, $1, 0); add_child($$, $2, 1); add_child($$, $3, 2);
		$$ -> flags = FLAG_ARGS_MORE;
	} | Exp {
		$$ = create_node(NODE_ARGS, $1 -> line);
		$$ -> flags = FLAG_ARGS_SINGLE;
		add_child($$, $1, 0);
	}
	;

%%


