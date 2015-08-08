%{
	#include "syntax.tab.h"
	#include "tree.h"
	#define min(a,b) ( ((a) < (b)) ? (a) : (b) )
	float f_tmp = 0;
%}

LETTER_		[A-Za-z_]
DIGIT		[0-9]

%%


\n		{lineno++;}
[ \t\r]	{/* white space */}
;		{/* printf("semicolon\n"); */
	yylval.type_node = createNode("Semicolon", lineno);
	return SEMI;
}

,		{/* printf("comma\n"); */
	yylval.type_node = createNode("Comma", lineno);
	return COMMA;
}
(>|<|>=|<=|==|!=) {/* printf("relational operator %s\n", yytext); */ 
	yylval.type_node = createNode("Relation operator", lineno);
	strncpy(yylval.type_node -> idName, yytext, strlen(yytext));
	return RELOP;
}

=		{/* printf("assignment operator\n"); */
	yylval.type_node = createNode("Assignment operator", lineno);
	return ASSIGNOP;
}

\*		{/* printf("star character\n"); */
	yylval.type_node = createNode("Star", lineno);
	return STAR;
}

\/		{/* printf("division operator\n)"); */
	yylval.type_node = createNode("Divisor", lineno);
	return DIV;
}
&&		{/* printf("logical and operator\n"); */
	yylval.type_node = createNode("And operator", lineno);
	return AND;
}

\|\|	{/* printf("logical or operator\n"); */
	yylval.type_node = createNode("Or operator", lineno);
	return OR;
}
\.		{/* printf("dot\n"); */
	yylval.type_node = createNode("Dot operator", lineno);
	return DOT;
}

!		{/* printf("bitwise / logical not operator\n"); */ 
	yylval.type_node = createNode("Not operator", lineno);
	return NOT;
}
int		{/* printf("data type INT\n"); */
	yylval.type_node = createNode("Integer type", lineno);
	return TYPE;
}
float	{/* printf("data type FLOAT\n"); */
	yylval.type_node = createNode("Float type", lineno);
	return TYPE;
}
\(		{/* printf("left parenthese\n"); */
	yylval.type_node = createNode("Left parenthese", lineno);
	return LP;
}
\)		{/* printf("right parenthese\n"); */
	yylval.type_node = createNode("Right parenthese", lineno);
	return RP;
}
\[		{/* printf("left bracket\n"); */
	yylval.type_node = createNode("Left bracket", lineno);
	return LB;
}
\]		{/* printf("right bracket\n"); */
	yylval.type_node = createNode("Right bracket", lineno);
	return RB;
}
\{		{/* printf("left cruly brace\n"); */
	yylval.type_node = createNode("Left curly brace", lineno);
	return LC;
}
\}		{/* printf("curly braces\n"); */
	yylval.type_node = createNode("Right curly brace", lineno);
	return RC;
}
struct	{/* printf("keyword STURCT\n"); */
	yylval.type_node = createNode("Keyword STRUCT", lineno);
	return STRUCT;
}
return	{/* printf("keyword RETURN\n"); */
	yylval.type_node = createNode("Keyword RETURN", lineno);
	return RETURN;
}
if		{/* printf("keyword IF\n"); */
	yylval.type_node = createNode("Keyword IF", lineno);
	return IF;
}
else	{/* printf("keyword ELSE\n"); */
	yylval.type_node = createNode("Keyword ELSE", lineno);
	return ELSE;
}
while	{/* printf("keyword WHILE\n"); */
	yylval.type_node = createNode("Keyword WHILE", lineno);
	return WHILE;
}



(\.{DIGIT}+)([eE][-+]?{DIGIT}+)? {
/*	printf("literate float-point number: %s\n", yytext); */
	yylval.type_node = createNode("Literate float number", lineno);
	sscanf(yytext, "%f", &f_tmp);
	yylval.type_node -> f_value = f_tmp;
	yylval.type_node -> flags |= FLAG_FLOAT;
	return FLOAT;
}

{DIGIT}+\.{DIGIT}*([eE][-+]?{DIGIT}+)? {
/*	printf("literate float-point number: %s\n", yytext); */
	yylval.type_node = createNode("Literate float number", lineno);
	sscanf(yytext, "%f", &f_tmp);
	yylval.type_node -> f_value = f_tmp;
	yylval.type_node -> flags |= FLAG_FLOAT;
	return FLOAT;
}

0|[1-9][0-9]*	{
	/* printf("literate integer number: %s\n", yytext); */
	yylval.type_node = createNode("Literate integer number", lineno);
	sscanf(yytext, "%d", &(yylval.type_node->i_value));
	yylval.type_node -> flags |= FLAG_INT;
	return INT;
}

{LETTER_}({LETTER_}|{DIGIT})*	{
	/* printf("identifier %s\n", yytext); */
	yylval.type_node = createNode("Identifier", lineno);
	strncpy(yylval.type_node -> idName, yytext, min(strlen(yytext), 31));
	yylval.type_node -> flags |= FLAG_ID;
	return ID;
}

\+		{/* printf("plus operator\n"); */
	yylval.type_node = createNode("PLUS", lineno);
	return PLUS;
}
-		{/* printf("minus operator\n"); */
	yylval.type_node = createNode("MINUS", lineno);
	return MINUS;
}

({DIGIT}*\.{DIGIT}*)[eE] {
	lexerror("invalid identifier: %s\n", yytext);
	yylval.type_node = createNode("Undefined", lineno);
	return UNDEF;
}

({DIGIT}|{LETTER_})+ {
	lexerror("Unrecognizable identifier: '%s'\n", yytext);
	return ID;
}

. {
	lexerror("Unrecognizable identifier: \'%s\'\n", yytext);
}

%%

