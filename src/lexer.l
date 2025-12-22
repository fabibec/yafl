%option noinput nounput noyywrap yylineno
%{
    #include "parser.h"
    #include "stringbuf.h"
    #include "utils.h"
    #include <stdio.h>
    #include <stdlib.h>
    #include <stdint.h>
    #include <inttypes.h>
%}

/* Patterns */
HEX_DIGIT [0-9A-Fa-f]

BASE (2|8|10|12)
BIN_NUM (0|1[0-1]*)
OCT_NUM (0|[1-7][0-7]*)
DEC_NUM (0|[1-9][0-9]*)
HEX_NUM (0|[1-9A-Fa-f][0-9A-Fa-f]*)

VAR_IDENT "|"[^|\t\n\r]+"|"
IDENT [a-zA-Z_][a-zA-Z0-9_]*

IGNORE [ \t\n]+

/* States */
%x BLOCK_COMMENT
%x STRING_STATE

%%

"~~".* { /* Ignore line-comment */ }
"{-" { BEGIN(BLOCK_COMMENT); }
<BLOCK_COMMENT>"-}" { BEGIN(INITIAL); }
<BLOCK_COMMENT>(.|\n) { /* Ignore block-comment */ }


"Yes" {
    yylval.nr = 1;
    return L_BOOL;
}
"No" {
    yylval.nr = 0;
    return L_BOOL;
}


"2#"{BIN_NUM} {
    yylval.nr = strtol(yytext + 2, NULL, 2);
    return L_INT;
}
"8#"{OCT_NUM} {
    yylval.nr = strtol(yytext + 2, NULL, 8);
    return L_INT;
}
"10#"{DEC_NUM} {
    yylval.nr = strtol(yytext + 3, NULL, 10);
    return L_INT;
}
"#"{HEX_NUM} {

    yylval.nr = strtol(yytext + 1, NULL, 16);
    return L_INT;
}
"16#"{HEX_NUM} {

    yylval.nr = strtol(yytext + 3, NULL, 16);
    return L_INT;
}
{BASE}"#"{HEX_NUM} {
    /* Catch-all for invalid numbers */
    fprintf(stderr, "Error in line %d: Invalid number format '%s'\n", yylineno, yytext);
    exit(1);
}
[0-9]+"#"[0-9A-Fa-f]+ {
    /* Catch-all for invalid base */
    fprintf(stderr, "Error line %d: Invalid base in number '%s' (use 2#, 8#, 10#, or 16#)\n", yylineno, yytext);
    exit(1);
}


">>" {
    BEGIN(STRING_STATE);
}
<STRING_STATE>"^x"{HEX_DIGIT}{HEX_DIGIT} {
    long val = strtol(yytext + 2, NULL, 16);
    chr_append((char) val);
}
<STRING_STATE>"^<<" { str_append("<<", 2); }
<STRING_STATE>"^n" { chr_append('\n'); }
<STRING_STATE>"<<" {
    yylval.str = strdup(str_buffer ? str_buffer : "");
    str_reset();
    BEGIN(INITIAL);
    return L_STR;
}
<STRING_STATE>(.|\n) { str_append(yytext, yyleng); }


"fn" { return KW_FN; }
"ret" { return KW_RET; }

"if" { return KW_IF; }
"else"  { return KW_ELSE; }
"while" { return KW_WHILE; }
"for" { return KW_FOR; }
"in" { return KW_IN; }
"range" { return KW_RANGE; }

"print" { return KW_PRINT; }

"->" { return S_RARROW; }
"<-" { return S_LARROW; }

"str" { return T_STR; }
"bool" { return T_BOOL; }
"none" { return T_NONE; }
"int"|"sint" { return T_SINT; }
"uint" { return T_UINT; }

{VAR_IDENT} {
    yylval.str = strip_pipes(yytext);
    return ID_VAR;
}
{IDENT} {
    yylval.str = strdup(yytext);
    return ID;
}

{IGNORE} ;

. { return *yytext; }

%%

/*int main(){
    while(yylex());
    return 0;
}*/

