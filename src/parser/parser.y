%code requires {
    #include <stdint.h>
    #include "ast.h"
    #include "types.h"
}

%{
    #include <stdio.h>
    #include <stdint.h>
    #include <inttypes.h>
    #include <string.h>
    #include "ast.h"
    #include "logger.h"

    extern int yylineno;

    int yylex();
    void yyerror(const char *msg){
        log_error(yylineno, msg);
    }

    /* Global AST Root */
    ast_node *root = NULL;
%}

%define parse.error verbose

%union {
    int nr;
    double fl_nr;
    char *str;
    ast_node *node;
    /* Type info */
    yafl_t *type;
}

/* Lexer Tokens */
%token <nr> L_INT L_BOOL
%token <fl_nr> L_FLOAT
%token <str> L_STR ID ID_VAR
%token KW_FN KW_RET KW_IF KW_ELIF KW_ELSE KW_WHILE KW_FOR KW_IN KW_NEXT KW_STOP
%token KW_WHERE
%token KW_MATCH KW_CASE KW_DEFAULT
%token S_RARROW S_LARROW
%token S_LARROW_ADD S_LARROW_SUB S_LARROW_MUL S_LARROW_DIV S_LARROW_MOD
%token OP_UN_DEC OP_UN_INC
%token OP_BIN_LE OP_BIN_GE OP_BIN_NE
%token OP_BIN_AND OP_BIN_OR
%token T_STR T_BOOL T_NONE T_SINT T_FLOAT T_RANGE
%token T_ARR

/* Non-terminals */
%type <node> program top_level_list top_level_item
%type <node> fn_definition fn_signature
%type <node> param_list param_list_nonempty param
%type <node> compound_stmt statement_list statement
%type <node> var_decl_stmt assignment_stmt return_stmt
%type <node> if_stmt opt_else for_stmt while_stmt next_stmt stop_stmt
%type <node> match_stmt case_list case_item
%type <node> for_loop_var
%type <node> expr primary call_expr arr_expr
%type <node> expr_list expr_list_opt
%type <type> type_specifier type_basic type_complex

/* Precedence */
%left OP_BIN_OR
%left OP_BIN_AND
%left '=' OP_BIN_NE
%left '<' '>' OP_BIN_LE OP_BIN_GE
%left '+' '-'
%left '*' '/' '%'
%precedence NOT NEG
%precedence OP_UN_INC OP_UN_DEC

%%

program:
    top_level_list { root = $1; }
    ;

top_level_list:
    top_level_list top_level_item { $$ = ast_append($1, $2); }
    | top_level_item
    ;

top_level_item:
    fn_definition
    | var_decl_stmt /* Global vars */
    ;

/* --- FUNCTIONS --- */
fn_definition:
    fn_signature compound_stmt {
        /* body added here */
        $1->data.func.body = $2;
        $$ = $1;
    }
    ;

fn_signature:
    KW_FN ID '(' param_list ')' S_RARROW type_specifier {
        /* body inserted in next step */
        $$ = ast_new_func($2, $4, $7, NULL);
    }
    ;

param_list:
    %empty { $$ = NULL; }
    | param_list_nonempty
    ;

param_list_nonempty:
    param
    | param_list_nonempty ',' param { $$ = ast_append($1, $3); }
    ;

param:
    type_specifier ':' ID_VAR {
        $$ = ast_new_param($3, $1, NULL);
    }
    | type_specifier ':' ID_VAR S_LARROW expr {
        $$ = ast_new_param($3, $1, $5);
    }
    ;

/* --- STATEMENTS --- */
compound_stmt:
    '{' statement_list '}' {
        $$ = ast_new_node(NODE_BLOCK);
        $$->data.block.stmts = $2;
    }
    ;

statement_list:
    statement_list statement { $$ = ast_append($1, $2); }
    | %empty { $$ = NULL; }
    ;

statement:
    compound_stmt
    | var_decl_stmt
    | assignment_stmt
    | return_stmt
    | if_stmt
    | for_stmt
    | while_stmt
    | next_stmt
    | stop_stmt
    | match_stmt
    | expr ';'
    ;

/* --- SPECIFIC STATEMENTS --- */
match_stmt:
    KW_MATCH expr '{' case_list '}' {
        $$ = ast_new_match($2, $4);
    }
    ;

case_list:
    case_list case_item { $$ = ast_append($1, $2); }
    | case_item
    ;

case_item:
    KW_CASE expr S_RARROW compound_stmt {
        $$ = ast_new_case($2, $4);
    }
    | KW_CASE expr S_RARROW {
        /* Empty body -> Fallthrough (represented by NULL body) */
        $$ = ast_new_case($2, NULL);
    }
    | KW_DEFAULT S_RARROW compound_stmt {
        $$ = ast_new_case(NULL, $3); /* Default case has NULL expr */
    }
    ;

var_decl_stmt:
    type_specifier ID_VAR S_LARROW expr ';' {
        $$ = ast_new_decl($1, $2, $4);
    }
    | type_specifier ID_VAR ';' {
        /* automatic zero-inits */
        $$ = ast_new_decl($1, $2, NULL);
    }
    | type_specifier ID_VAR '[' expr ']' ';' {
        if ($1->base_t != TYPE_ARR) {
            yyerror("Array size declaration only valid for array types");
            $$ = NULL;
        } else {
            // arr'int a[5] -> a = [default(int)] * 5
            yafl_t *inner = type_clone($1->comp_t);
            ast_node *def_val = ast_new_default(inner);
            ast_node *fill_expr = ast_new_arr_fill(def_val, $4);
            $$ = ast_new_decl($1, $2, fill_expr);
        }
    }
    ;

assignment_stmt:
    ID_VAR S_LARROW expr ';' {
        $$ = ast_new_assign($1, $3);
    }
    | ID_VAR S_LARROW_ADD expr ';' {
        $$ = ast_new_assign($1, ast_new_binary(OP_ADD, ast_new_var(strdup($1)), $3));
    }
    | ID_VAR S_LARROW_SUB expr ';' {
        $$ = ast_new_assign($1, ast_new_binary(OP_SUB, ast_new_var(strdup($1)), $3));
    }
    | ID_VAR S_LARROW_MUL expr ';' {
        $$ = ast_new_assign($1, ast_new_binary(OP_MUL, ast_new_var(strdup($1)), $3));
    }
    | ID_VAR S_LARROW_DIV expr ';' {
        $$ = ast_new_assign($1, ast_new_binary(OP_DIV, ast_new_var(strdup($1)), $3));
    }
    | ID_VAR S_LARROW_MOD expr ';' {
        $$ = ast_new_assign($1, ast_new_binary(OP_MOD, ast_new_var(strdup($1)), $3));
    }
    | primary '[' expr ']' S_LARROW expr ';' {
        $$ = ast_new_arr_assign($1, $3, $6);
    }
    ;

return_stmt:
    KW_RET expr ';' {
        $$ = ast_new_ret($2);
    }
    | KW_RET ';' {
        $$ = ast_new_ret(NULL);
    }
    ;

/* --- IF with optional ELSE --- */
if_stmt:
    KW_IF '(' expr ')' compound_stmt opt_else {
        $$ = ast_new_if($3, $5, $6);
    }
    ;

opt_else:
    KW_ELSE compound_stmt { $$ = $2; }
    | KW_ELIF '(' expr ')' compound_stmt opt_else {
        $$ = ast_new_if($3, $5, $6);
    }
    | %empty { $$ = NULL; }
    ;

/* --- FOR loops --- */
for_stmt:
    KW_FOR for_loop_var KW_IN expr compound_stmt {
        $$ = ast_new_for($2, $4, $5);
    }
    ;

for_loop_var:
    type_basic ID_VAR {
        $$ = ast_new_node(NODE_FOR_DECL);
        $$->data.for_decl.type = $1;
        $$->data.for_decl.name = $2;
    }
    | ID_VAR {
        $$ = ast_new_node(NODE_FOR_VAR);
        $$->data.for_var.name = $1;
    }
    ;

while_stmt:
KW_WHILE '(' expr ')' compound_stmt {
    $$ = ast_new_while($3, $5);
}

next_stmt:
KW_NEXT ';' {
    $$ = ast_new_next();
}

stop_stmt:
KW_STOP ';' {
    $$ = ast_new_stop();
}    ;

/* --- expressions --- */
expr:
    expr '+' expr { $$ = ast_new_binary(OP_ADD, $1, $3); }
    | expr '-' expr { $$ = ast_new_binary(OP_SUB, $1, $3); }
    | expr '*' expr {
        if ($1->type == NODE_ARR_LIT) {
            $$ = ast_new_arr_fill($1->data.arr_lit.elements, $3);
            $1->data.arr_lit.elements = NULL;
            ast_free($1);
        } else {
            $$ = ast_new_binary(OP_MUL, $1, $3);
        }
    }
    | expr '/' expr { $$ = ast_new_binary(OP_DIV, $1, $3); }
    | expr '%' expr { $$ = ast_new_binary(OP_MOD, $1, $3); }
    | expr '<' expr { $$ = ast_new_binary(OP_LT, $1, $3); }
    | expr '>' expr { $$ = ast_new_binary(OP_GT, $1, $3); }
    | expr '=' expr { $$ = ast_new_binary(OP_EQ, $1, $3); }
    | expr OP_BIN_LE expr { $$ = ast_new_binary(OP_LE, $1, $3); }
    | expr OP_BIN_GE expr { $$ = ast_new_binary(OP_GE, $1, $3); }
    | expr OP_BIN_NE expr { $$ = ast_new_binary(OP_NE, $1, $3); }
    | expr OP_BIN_AND expr { $$ = ast_new_binary(OP_AND, $1, $3); }
    | expr OP_BIN_OR expr { $$ = ast_new_binary(OP_OR, $1, $3); }
    | '~' expr %prec NOT { $$ = ast_new_unary(OP_NOT, $2); }
    | '-' expr %prec NEG { $$ = ast_new_unary(OP_NEG, $2); }
    | expr OP_UN_INC { $$ = ast_new_unary(OP_INC, $1); }
    | expr OP_UN_DEC { $$ = ast_new_unary(OP_DEC, $1); }
    | '(' expr ')' { $$ = $2; }
    | primary
    ;

primary:
    L_INT { $$ = ast_new_int($1); }
    | L_FLOAT { $$ = ast_new_float($1); }
    | L_STR { $$ = ast_new_str($1); }
    | L_BOOL { $$ = ast_new_bool($1); }
    | ID_VAR { $$ = ast_new_var($1); }
    | call_expr
    | arr_expr
    ;

arr_expr:
    '[' expr_list ']' {
        $$ = ast_new_arr_lit($2);
    }
    | primary '[' expr ']' {
        $$ = ast_new_arr_idx($1, $3);
    }
    ;

call_expr:
    ID '(' expr_list_opt ')' {
        $$ = ast_new_call($1, $3);
    }
    ;

expr_list_opt:
    expr_list
    | %empty { $$ = NULL; }
    ;

expr_list:
    expr_list ',' expr { $$ = ast_append($1, $3); }
    | expr
    ;


/* --- TYPES --- */
type_specifier:
    type_basic
    | type_complex
    ;

type_basic:
    T_STR { $$ = type_new_simple(TYPE_STR); }
    | T_BOOL { $$ = type_new_simple(TYPE_BOOL); }
    | T_SINT { $$ = type_new_simple(TYPE_SINT); }
    | T_FLOAT { $$ = type_new_simple(TYPE_FLOAT); }
    ;

type_complex:
    T_NONE { $$ = type_new_simple(TYPE_VOID); }
    | T_ARR '\'' type_specifier {
        $$ = type_new_composite($3);
    }
    | T_RANGE { $$ = type_new_simple(TYPE_RANGE); }
    ;
%%
