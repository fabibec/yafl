%option noinput nounput noyywrap yylineno
%{
    #include "parser.h"
    #include "stringbuf.h"
    #include <stdio.h>
    #include <stdlib.h>
    #include <stdint.h>
    #include <inttypes.h>
%}

/* Patterns */
BASE (2|8|10|16)
DEC_NUM (0|[1-9][0-9]*)
HEX [0-9A-Fa-f]
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


"true"{
    yylval.nr = 1;
    return L_BOOL;
}
"false"{
    yylval.nr = 0;
    return L_BOOL;
}


{DEC_NUM} {
    yylval.nr = strtoull(yytext, NULL, 10);
    return L_INT;
}
"#"{HEX_NUM} {
    yylval.nr = strtoull(yytext + 1, NULL, 16);
    return L_INT;
}
{BASE}"#"{NUMBER} {
    char* sep = strchr(yytext, '#');
    *sep = '\0';
    int base = atoi(yytext);
    yylval.nr = strtol(sep + 1, NULL, base);
    *sep = '#';
    return L_INT;
}


">>" {
    BEGIN(STRING_STATE);
}
<STRING_STATE>"^x"{HEX}{HEX} {
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
<STRING_STATE>.* { str_append(yytext, yyleng); }


{VAR_IDENT} {
    yylval.str = strip_pipes(yytext);
    return ID_VAR;
}
{IDENT} {
    yylval.str = strdup(yytext);
    return ID;
}

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

"chr" { return T_CHAR; }
"str" { return T_STR; }
"bool" { return T_BOOL; }
"none" { return T_NONE; }

"int8"|"sint8" { return T_SINT8; }
"int16"|"sint16" { return T_SINT16; }
"int"|"sint"|"sint32" { return T_SINT32; }
"sint64" { return T_SINT64; }

"uint8" { return T_UINT8; }
"uint16" { return T_UINT16; }
"uint"|"uint32" { return T_UINT32; }
"uint64" { return T_UINT64; }

{IGNORE} ;

. { return *yytext; }

%%

/*int main(){
    while(yylex());
    return 0;
}*/

