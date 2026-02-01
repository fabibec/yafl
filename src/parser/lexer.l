%option noinput nounput noyywrap yylineno
%{
    #include "parser.h"
    #include "stringbuf.h"
    #include "utils.h"
    #include "logger.h"
    #include <limits.h>
    #include <math.h>
    #include <stdio.h>
    #include <stdlib.h>
%}

/* Patterns */
HEX_DIGIT [0-9A-Fa-f]

BASE [0-9]+
DEC_NUM (0|[1-9]([0-9_]*[0-9])?)
DEC_PART [0-9]([0-9_]*[0-9])?
HEX_NUM (0|[1-9A-Fa-f]([0-9A-Fa-f_]*[0-9A-Fa-f])?)
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

"not" { return OP_UN_NOT; }
"++" { return OP_UN_INC; }
"--" { return OP_UN_DEC; }

"Yes" {
    yylval.nr = 1;
    return L_BOOL;
}
"No" {
    yylval.nr = 0;
    return L_BOOL;
}

"NaN" {
    yylval.fl_nr = NAN;
    return L_FLOAT;
}
"Inf" {
    yylval.fl_nr = INFINITY;
    return L_FLOAT;
}

">>" {
    BEGIN(STRING_STATE);
}
<STRING_STATE>"^x"{HEX_DIGIT}{HEX_DIGIT} {
    long val = strtol(yytext + 2, NULL, 16);
    chr_append((char) val);
}
<STRING_STATE>"^<<" { str_append("<<", 2); }
<STRING_STATE>"^^" { chr_append('^'); }
<STRING_STATE>"^n" { chr_append('\n'); }
<STRING_STATE>"^t" { chr_append('\t'); }
<STRING_STATE>"<<" {
    yylval.str = strdup(str_buffer ? str_buffer : "");
    str_reset();
    BEGIN(INITIAL);
    return L_STR;
}
<STRING_STATE>(.|\n) { str_append(yytext, yyleng); }

"fn" { return KW_FN; }
"ret" { return KW_RET; }

"match" { return KW_MATCH; }
"case" { return KW_CASE; }
"otherwise" { return KW_OTHERWISE; }

"if" { return KW_IF; }
"elif" { return KW_ELIF; }
"else"  { return KW_ELSE; }
"while" { return KW_WHILE; }
"for" { return KW_FOR; }
"next" { return KW_NEXT; }
"stop" { return KW_STOP; }
"in" { return KW_IN; }

"<-+" { return S_LARROW_ADD; }
"<--" { return S_LARROW_SUB; }
"<-*" { return S_LARROW_MUL; }
"<-/" { return S_LARROW_DIV; }
"<-%" { return S_LARROW_MOD; }
"->" { return S_RARROW; }
"<-" { return S_LARROW; }

"arr" { return T_ARR; }
"str" { return T_STR; }
"bool" { return T_BOOL; }
"none" { return T_NONE; }
"int"  { return T_SINT; }
"float" { return T_FLOAT; }
"range\'int" { return T_RANGE; }

{BASE}"#"{HEX_NUM} {
    /* Based int */
    int base = parse_base(yytext);
    int v;
    if(!base){
        log_error(yylineno, "Invalid base for integer number '%s' (base >= 2 and base <= 16).", yytext);
    }
    char *s = (base >= 10) ? yytext + 3 : yytext + 2;
    if(!parse_based_int(base, s, &v)){
        log_warn(yylineno, "Integer literal '%s' overflowed (wrapped).", s);
    }
    yylval.nr = v;
    return L_INT;
}

{DEC_NUM} {
    /* Decimal int */
    int v;
    if(!parse_based_int(10, yytext, &v)){
        log_warn(yylineno, "Integer literal '%s' overflowed (wrapped).", yytext);
    }
    yylval.nr = v;
    return L_INT;
}
"#"{HEX_NUM} {
    /* Hexdecimal int */
    int v;
    if(!parse_based_int(16, yytext + 1, &v)){
        log_warn(yylineno, "Integer literal '%s' overflowed (wrapped).", yytext);
    }
    yylval.nr = v;
    return L_INT;
}

{BASE}"#"{HEX_NUM}"."{HEX_NUM}("^"{EXPONENT})? {
    /* Based float */
    int base = parse_base(yytext);
    if(!base){
        log_error(yylineno, "Invalid base for float number '%s' (base >= 2 and base <= 16).", yytext);
    }
    char *s = (base >= 10) ? yytext + 3 : yytext + 2;
    double v;
    if(!parse_based_float(base, s, &v)){
        log_error(yylineno, "Invalid float literal '%s'.", s);
    }
    if(isinf(v)){
         log_warn(yylineno, "Float literal '%s' overflowed (infinity).", s);
    }
    yylval.fl_nr = v;
    return L_FLOAT;
}
{DEC_NUM}"."{DEC_PART}("^"{EXPONENT})? {
    /* Decimal float */
    double v;
    if(!parse_based_float(10, yytext, &v)){
        log_error(yylineno, "Invalid float literal '%s'.", yytext);
    }
    if(isinf(v)){
         log_warn(yylineno, "Float literal '%s' overflowed (infinity).", yytext);
    }
    yylval.fl_nr = v;
    return L_FLOAT;
}
"#"{HEX_NUM}"."{HEX_NUM}("^"{EXPONENT})? {
    /* Hexdecimal float */
    double v;
    if(!parse_based_float(16, yytext + 1, &v)){
        log_error(yylineno, "Invalid hex float '%s'.", yytext);
    }
    if(isinf(v)){
         log_warn(yylineno, "Float literal '%s' overflowed (infinity).", yytext);
    }
    yylval.fl_nr = v;
    return L_FLOAT;
}

{VAR_IDENT} {
    yylval.str = strip_pipes(yytext);
    return ID_VAR;
}

"_" {
    log_error(yylineno, "Invalid identifier '_'.");
}
{IDENT} {
    yylval.str = strdup(yytext);
    return ID;
}

{IGNORE} ;

. { return *yytext; }

%%
