%{
	#include "syntax.tab.h"
	#include "utilities/tree.h"
	#include "utilities/common.h"
	#define min(a,b) ( ((a) < (b)) ? (a) : (b) )
	float f_tmp = 0;
	int relop_parser(const char *);

%}

LETTER_		[A-Za-z_]
DIGIT		[0-9]

%%


\n		{lineno++;}
[ \t\r]	{/* white space */}
;		{/* printf("semicolon\n"); */
	yylval.type_node = create_node(TOKEN_SEMICOLON, lineno);
	return SEMI;
}


,		{/* printf("comma\n"); */
	yylval.type_node = create_node(TOKEN_COMMA, lineno);
	return COMMA;
}
(>|<|>=|<=|==|!=) {/* printf("relational operator %s\n", yytext); */ 
	yylval.type_node = create_node(TOKEN_RELOP, lineno);
	yylval.type_node -> relop_type = relop_parser(yytext);
	return RELOP;
}

=		{/* printf("assignment operator\n"); */
	yylval.type_node = create_node(TOKEN_ASSIGNOP, lineno);
	return ASSIGNOP;
}

\*		{/* printf("star character\n"); */
	yylval.type_node = create_node(TOKEN_STAR, lineno);
	return STAR;
}

\/		{/* printf("division operator\n)"); */
	yylval.type_node = create_node(TOKEN_DIV, lineno);
	return DIV;
}
&&		{/* printf("logical and operator\n"); */
	yylval.type_node = create_node(TOKEN_AND, lineno);
	return AND;
}

\|\|	{/* printf("logical or operator\n"); */
	yylval.type_node = create_node(TOKEN_OR, lineno);
	return OR;
}
\.		{/* printf("dot\n"); */
	yylval.type_node = create_node(TOKEN_DOT, lineno);
	return DOT;
}

!		{/* printf("bitwise / logical not operator\n"); */ 
	yylval.type_node = create_node(TOKEN_NOT, lineno);
	return NOT;
}
int		{/* printf("data type INT\n"); */
	yylval.type_node = create_node(TOKEN_TYPE, lineno);
	yylval.type_node -> unit_name = TOKEN_TYPE;
	yylval.type_node -> flags = FLAG_TYPE_INT;
	yylval.type_node -> type_name = TYPE_INT;
	return TYPE;
}
float	{/* printf("data type FLOAT\n"); */
	yylval.type_node = create_node(TOKEN_TYPE, lineno);
	yylval.type_node -> unit_name = TOKEN_TYPE;
	yylval.type_node -> flags = FLAG_TYPE_FLOAT;
	yylval.type_node -> type_name = TYPE_FLOAT;
	return TYPE;
}
\(		{/* printf("left parenthese\n"); */
	yylval.type_node = create_node(TOKEN_PAREN, lineno);
	yylval.type_node -> pbc_part = PBC_LEFT;
	return LP;
}
\)		{/* printf("right parenthese\n"); */
	yylval.type_node = create_node(TOKEN_PAREN, lineno);
	yylval.type_node -> pbc_part = PBC_RIGHT;
	return RP;
}
\[		{
	yylval.type_node = create_node(TOKEN_BRACKET, lineno);	
	yylval.type_node -> pbc_part = PBC_LEFT;
	return LB;
}
\]		{/* printf("right bracket\n"); */
	yylval.type_node = create_node(TOKEN_BRACKET, lineno);
	yylval.type_node -> pbc_part = PBC_RIGHT;
	return RB;
}
\{		{/* printf("left cruly brace\n"); */
	yylval.type_node = create_node(TOKEN_CURLYBRACE, lineno);
	yylval.type_node -> pbc_part = PBC_LEFT;
	return LC;
}
\}		{/* printf("curly braces\n"); */
	yylval.type_node = create_node(TOKEN_CURLYBRACE, lineno);
	yylval.type_node -> pbc_part = PBC_RIGHT;
	return RC;
}
struct	{/* printf("keyword STURCT\n"); */
	yylval.type_node = create_node(TOKEN_STRUCT, lineno);
	return STRUCT;
}
return	{/* printf("keyword RETURN\n"); */
	yylval.type_node = create_node(TOKEN_RETURN, lineno);
	return RETURN;
}
if		{/* printf("keyword IF\n"); */
	yylval.type_node = create_node(TOKEN_IF, lineno);
	return IF;
}
else	{/* printf("keyword ELSE\n"); */
	yylval.type_node = create_node(TOKEN_ELSE, lineno);
	return ELSE;
}
while	{/* printf("keyword WHILE\n"); */
	yylval.type_node = create_node(TOKEN_WHILE, lineno);
	return WHILE;
}



(\.{DIGIT}+)([eE][-+]?{DIGIT}+)? {
/*	printf("literate float-point number: %s\n", yytext); */
	yylval.type_node = create_node(TOKEN_FLOAT, lineno);
	sscanf(yytext, "%f", &f_tmp);
	yylval.type_node -> f_value = f_tmp;
	return FLOAT;
}

{DIGIT}+\.{DIGIT}*([eE][-+]?{DIGIT}+)? {
/*	printf("literate float-point number: %s\n", yytext); */
	yylval.type_node = create_node(TOKEN_FLOAT, lineno);
	sscanf(yytext, "%f", &f_tmp);
	yylval.type_node -> f_value = f_tmp;
	return FLOAT;
}

0|[1-9][0-9]*	{
	/* printf("literate integer number: %s\n", yytext); */
	yylval.type_node = create_node(TOKEN_INTEGER, lineno);
	sscanf(yytext, "%d", &(yylval.type_node->i_value));
	return INT;
}

{LETTER_}({LETTER_}|{DIGIT})*	{
	/* printf("identifier %s\n", yytext); */
	yylval.type_node = create_node(TOKEN_IDENTIFIER, lineno);
	yylval.type_node -> id_extra = (id_extra_t*)malloc(sizeof(id_extra_t));
	memset(yylval.type_node -> id_extra, 0, sizeof(id_extra_t));
	strncpy(yylval.type_node -> id_extra -> name, yytext, min(strlen(yytext), 31));
	//printf("lexical: \'%s\'\n", yylval.type_node -> id_extra -> name);
	return ID;
}

\+		{/* printf("plus operator\n"); */
	yylval.type_node = create_node(TOKEN_PLUS, lineno);
	return PLUS;
}
-		{/* printf("minus operator\n"); */
	yylval.type_node = create_node(TOKEN_MINUS, lineno);
	return MINUS;
}


({DIGIT}|{LETTER_})+ {
	lexerror("Unrecognizable identifier: '%s'\n", yytext);
	return ID;
}

. {
	lexerror("Unrecognizable identifier: \'%s\'\n", yytext);
}

%%

int relop_parser(const char *str) {
	if (strlen(str) == 1) {
		if (str[0] == '<') return RELOP_LT;
		if (str[0] == '>') return RELOP_GT;
		assert(0);
	} else if (strlen(str) == 2) {
		if (str[0] == '=') return RELOP_EQ;
		if (str[0] == '!') return RELOP_NE;
		if (str[0] == '<') return RELOP_LE;
		if (str[0] == '>') return RELOP_GE;
		assert(0);
	}
	lexerror("Unrecognizable relop: \'%s\'\n", str);
	assert(0);
	return -1;
}
