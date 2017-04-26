%{
#include <string.h>
#include "util.h"
#include "tokens.h"
#include "errormsg.h"

#include <stdio.h>

int charPos=1;

int yywrap(void)
{
 charPos=1;
 return 1;
}


void adjust(void)
{
 EM_tokPos=charPos;
 charPos+=yyleng;
}

%}

id [a-zA-Z][a-z0-9A-Z_]*
integer [0-9]+
string \"((\\.)|(\\\n)|[^\"\\])*\"
comment "/*"([^\*]|(\*)*[^\*/])*(\*)*"*/"

%%
" "	 	{adjust(); continue;}
\n	 	{adjust(); EM_newline(); continue;}
[\t\r]	{adjust(); continue;}

","		{adjust(); return COMMA;}
":"		{adjust(); return COLON;}
";"		{adjust(); return SEMICOLON;}
"("		{adjust(); return LPAREN;}
")"		{adjust(); return RPAREN;}
"["		{adjust(); return LBRACK;}
"]"		{adjust(); return RBRACK;}
"{"		{adjust(); return LBRACE;}
"}"		{adjust(); return RBRACE;}
"."		{adjust(); return DOT;}
"+"		{adjust(); return PLUS;}
"-"		{adjust(); return MINUS;}
"*"		{adjust(); return TIMES;}
"/"		{adjust(); return DIVIDE;}
"="		{adjust(); return EQ;}
"<>"	{adjust(); return NEQ;}
"<"		{adjust(); return LT;}
"<="	{adjust(); return LE;}
">"		{adjust(); return GT;}
">="	{adjust(); return GE;}
"&"		{adjust(); return AND;}
"|"		{adjust(); return OR;}
":="	{adjust(); return ASSIGN;}

"while"	{adjust(); return WHILE;}
"for"	{adjust(); return FOR;}
"to"	{adjust(); return TO;}
"break"	{adjust(); return BREAK;}
"let"	{adjust(); return LET;}
"in"	{adjust(); return IN;}
"end"	{adjust(); return END;}
"function"	{adjust(); return FUNCTION;}
"var"	{adjust(); return VAR;}
"type"	{adjust(); return TYPE;}
"array"	{adjust(); return ARRAY;}
"if"	{adjust(); return IF;}
"then"	{adjust(); return THEN;}
"else"	{adjust(); return ELSE;}
"do"	{adjust(); return DO;}
"of"	{adjust(); return OF;}
"nil"	{adjust(); return NIL;}

{id}    {adjust(); yylval.sval = yytext; return ID;}
{integer}   {adjust(); yylval.ival = atoi(yytext); return INT;}
{string}   {adjust();
			yylval.sval = string_convert(yytext);
			for (const char *p = yytext; *p; p++) {
				if (*p == '\n') {
					EM_newline();
				}
			}
			return STRING;}
{comment}   {
			adjust();
			for (const char *p = yytext; *p; p++) {
				if (*p != '\n') {
					EM_newline();
				}
			}
			continue;
			}

.	 {adjust(); EM_error(EM_tokPos,"illegal token");}

%%
//convert escape characters to char
char *string_convert(const char *text) {
    char *str = (char *)malloc(strlen(text) * sizeof(char));
    char *pstr = str;
    const char *ptext = text;

    int state = 0;
    int ascii_tmp;

    while (*ptext != '\0') {
        switch (state) {
            case 0:
                if (*ptext == '\"') {
                    state = 1;
                }
                break;
            case 1:
                if (*ptext == '\\') {
                    state = 2;
                } else if (*ptext == '\"') {
                    state = 7;
                } else {
                    *(pstr++) = *ptext;
                    state = 1;
                }
                break;
            case 2:
                if (*ptext == 'n') {
                    *(pstr++) = 10;
                    state = 1;
                } else if (*ptext == 't') {
                    *(pstr++) = 9;
                    state = 1;
                } else if (*ptext == '\"') {
                    *(pstr++) = '\"';
                    state = 1;
                } else if (*ptext == '\\') {
                    *(pstr++) = '\\';
                    state = 1;
                } else if (*ptext == '^') {
                    state = 3;
                } else if (*ptext >= '0' && *ptext <= '9') {
                    ascii_tmp = *ptext - '0';
                    state = 4;
                } else if (*ptext == ' ' || *ptext == '\r' || *ptext == '\t' || *ptext == '\n') {
                    state = 6;
                } else {
                    //error;
					EM_error(EM_tokPos,"illegal escape character");
					state = 1;
                }
                break;
            case 3:
                break;
            case 4:
                if (*ptext >= '0' && *ptext <= '9') {
                    ascii_tmp = ascii_tmp * 10 + *ptext - '0';
                    state = 5;
                } else {
                    //err
					EM_error(EM_tokPos,"illegal escape character");
					state = 1;
                }
                break;
            case 5:
                if (*ptext >= '0' && *ptext <= '9') {
                    ascii_tmp = ascii_tmp * 10 + *ptext - '0';
                    if (ascii_tmp < 256) {
                        *(pstr++) = ascii_tmp;
                        state = 1;
                    } else {
                        //out of range
						EM_error(EM_tokPos,"character out of range");
						state = 1;
                    }
                } else {
                    //err
					EM_error(EM_tokPos,"illegal escape character");
					state = 1;
                }
                break;
            case 6:
                if (*ptext == ' ' || *ptext == '\r' || *ptext == '\n' || *ptext == '\t') {
                    state = 6;
                } else if (*ptext == '\\') {
                    state = 1;
                } else {
                    //err
					EM_error(EM_tokPos,"illegal escape character");
					state = 1;
                }
                break;
            case 7:
                //end state
                break;
        }
        ptext++;
    }

    *pstr = '\0';

    return str;
}