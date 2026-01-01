%option noinput nounput noyywrap yylineno
%{
    #include "parser.h"
    #include "stringbuf.h"
    #include "utils.h"
    #include <math.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <stdint.h>
    #include <inttypes.h>
    #include <limits.h>
%}

/* Patterns */
HEX_DIGIT [0-9A-Fa-f]

BASE [0-9]+
DEC_NUM (0|[1-9][0-9_]*)
HEX_NUM (0|[1-9A-Fa-f][0-9A-Fa-f_]*)
EXPONENT [+-]?[0-9]+

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

"<=" { return OP_BIN_LE; }
">=" { return OP_BIN_GE; }
"~=" { return OP_BIN_NE; }
"and" { return OP_BIN_AND; }
"or" { return OP_BIN_OR; }
"++" { return OP_UN_INC; }
"--" { return OP_UN_DEC; }

"MaxInt" {
    yylval.nr = INT_MAX;
    return L_INT;
}

"MinInt" {
    yylval.nr = INT_MIN;
    return L_INT;
}

"Yes" {
    yylval.nr = 1;
    return L_BOOL;
}
"No" {
    yylval.nr = 0;
    return L_BOOL;
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
"elif" { return KW_ELIF; }
"else"  { return KW_ELSE; }
"while" { return KW_WHILE; }
"where" { return KW_WHERE; }
"for" { return KW_FOR; }
"in" { return KW_IN; }

"->" { return S_RARROW; }
"<-" { return S_LARROW; }

"arr" { return T_ARR; }
"str" { return T_STR; }
"bool" { return T_BOOL; }
"none" { return T_NONE; }
"int"|"sint" { return T_SINT; }
"uint" { return T_UINT; }
"float" { return T_FLOAT; }
"range\'int" { return T_RANGE; }

{BASE}"#"{HEX_NUM} {
    /* Based int */
    int base = parse_base(yytext);
    int v;
    if(!base){
        fprintf(stderr, "Error line %d: Invalid base for integer number '%s' (base >= 2 and base <= 16)\n", yylineno, yytext);
        exit(1);
    }
    char *s = (base >= 10) ? yytext + 3 : yytext + 2;
    int ok = parse_based_int(base, s, &v);
    if(!ok){
        fprintf(stderr, "Error line %d: Invalid digit in integer number '%s' (every digit needs to be < %d)\n",
            yylineno, s, base);
        exit(1);
    }
    yylval.nr = v;
    return L_INT;
}

{HEX_NUM} {
    /* Decimal int with error handling */
    int v;
    int base = 10;
    int ok = parse_based_int(base, yytext, &v);
    if(!ok){
        fprintf(stderr, "Error line %d: Invalid digit in integer number '%s' (every digit needs to be < %d)\n",
            yylineno, yytext, base);
        exit(1);
    }
    yylval.nr = v;
    return L_INT;
}
"#"{HEX_NUM} {
    /* Hexdecimal int */
    int v;
    int base = 16;
    parse_based_int(base, yytext + 1, &v);
    /* Lexer already checks for errors */
    yylval.nr = v;
    return L_INT;
}

{BASE}"#"{HEX_NUM}("."{HEX_NUM})?("^"{EXPONENT})? {
    /* Based float */
    int base = parse_base(yytext);
    if(!base){
        fprintf(stderr, "Error in line %d: Invalid base for float number '%s' (base >= 2 and base <= 16)\n", yylineno, yytext);
        exit(1);
    }
    char *s = (base >= 10) ? yytext + 3 : yytext + 2;
    double v = parse_based_float(base, s);
    if(isnan(v)){
        fprintf(stderr, "Error in line %d: Invalid float literal '%s'\n", yylineno, s);
        exit(1);
    }
    yylval.fl_nr = v;
    return L_FLOAT;
}
{HEX_NUM}("."{HEX_NUM})?("^"{EXPONENT})? {
    /* Decimal float with error handling */
    double v = parse_based_float(10, yytext);
    if(isnan(v)){
        fprintf(stderr, "Error line %d: Invalid digit in float number '%s' (every digit needs to be < 10)\n",
            yylineno, yytext);
        exit(1);
    }
    yylval.fl_nr = v;
    return L_FLOAT;
}
"#"{HEX_NUM}("."{HEX_NUM})?("^"{EXPONENT})? {
    /* Hexdecimal float */
    double v = parse_based_float(16, yytext + 1);
    if(isnan(v)){
        fprintf(stderr, "Error line %d: Invalid hex float '%s'\n", yylineno, yytext);
        exit(1);
    }
    yylval.fl_nr = v;
    return L_FLOAT;
}

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
