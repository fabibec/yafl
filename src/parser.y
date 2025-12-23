%code requires {
    #include <stdint.h>
    #include "ast.h"
    #include "types.h"
}

%{
    #include <stdio.h>
    #include <stdint.h>
    #include <inttypes.h>
    #include "ast.h"

    extern int yylineno;

    int yylex();
    void yyerror(const char *msg){
        fprintf(stderr, "Error in line %d: %s\n", yylineno, msg);
    }

    /* Global AST Root */
    ast_node *root = NULL;
%}

%define parse.error verbose

%union {
    uint64_t nr;
    char *str;
    ast_node *node;
    /* Type info */
    yafl_t type;
}

/* Lexer Tokens */
%token <nr> L_INT L_BOOL
%token <str> L_STR ID ID_VAR
%token KW_FN KW_RET KW_IF KW_ELSE KW_WHILE KW_FOR KW_IN
%token KW_RANGE KW_PRINT KW_WHERE KW_TO
%token S_RARROW S_LARROW
%token OP_UN_DEC OP_UN_INC
%token OP_BIN_LE OP_BIN_GE OP_BIN_NE
%token OP_BIN_AND OP_BIN_OR
%token T_STR T_BOOL T_NONE T_SINT T_UINT

/* Non-terminals */
%type <node> program top_level_list top_level_item
%type <node> fn_definition fn_signature
%type <node> param_list param_list_nonempty param
%type <node> compound_stmt statement_list statement
%type <node> var_decl_stmt assignment_stmt return_stmt
%type <node> if_stmt opt_else for_stmt while_stmt print_stmt
%type <node> for_loop_var
%type <node> expr primary call_expr cast_expr
%type <node> expr_list expr_list_opt
%type <type> type_specifier for_loop_var_type

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
        $$ = ast_new_node(NODE_PARAM);
        $$->data.param.type = $1;
        $$->data.param.name = $3;
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
    var_decl_stmt
    | assignment_stmt
    | return_stmt
    | if_stmt
    | for_stmt
    | while_stmt
    | print_stmt
    | expr ';'
    ;

/* --- SPECIFIC STATEMENTS --- */
var_decl_stmt:
    type_specifier ID_VAR S_LARROW expr ';' {
        $$ = ast_new_decl($1, $2, $4);
    }
    | type_specifier ID_VAR ';' {
        /* automatic zero-inits */
        $$ = ast_new_decl($1, $2, NULL);
    }
    ;

assignment_stmt:
    ID_VAR S_LARROW expr ';' {
        $$ = ast_new_assign($1, $3);
    }
    ;

return_stmt:
    KW_RET expr ';' {
        $$ = ast_new_ret($2);
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
    | %empty { $$ = NULL; }
    ;

/* --- FOR loops --- */
for_stmt:
    /* range(end) - start=0, step=1 */
    KW_FOR for_loop_var KW_IN KW_RANGE '(' expr ')' compound_stmt {
        ast_node *start = ast_new_int(0);
        ast_node *step = ast_new_int(1);
        $$ = ast_new_for($2, start, $6, step, $8);
    }
    /* range(start, end) - step=1 */
    | KW_FOR for_loop_var KW_IN KW_RANGE '(' expr ',' expr ')' compound_stmt {
        ast_node *step = ast_new_int(1);
        $$ = ast_new_for($2, $6, $8, step, $10);
    }
    /* range(start, end, step) - fully specified */
    | KW_FOR for_loop_var KW_IN KW_RANGE '(' expr ',' expr ',' expr ')' compound_stmt {
        $$ = ast_new_for($2, $6, $8, $10, $12);
    }
    ;

for_loop_var_type:
    T_SINT { $$ = TYPE_SINT; }
    | T_UINT { $$ = TYPE_UINT; }
    ;

for_loop_var:
    for_loop_var_type ID_VAR {
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
    ;

print_stmt:
    KW_PRINT '(' expr ')' ';' {
        $$ = ast_new_print($3);
    }
    ;

/* --- expressions --- */
expr:
    expr '+' expr { $$ = ast_new_binary(OP_ADD, $1, $3); }
    | expr '-' expr { $$ = ast_new_binary(OP_SUB, $1, $3); }
    | expr '*' expr { $$ = ast_new_binary(OP_MUL, $1, $3); }
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
    | L_STR { $$ = ast_new_str($1); }
    | L_BOOL { $$ = ast_new_bool($1); }
    | ID_VAR { $$ = ast_new_var($1); }
    | call_expr
    | cast_expr
    ;

call_expr:
    ID '(' expr_list_opt ')' {
        $$ = ast_new_call($1, $3);
    }
    ;

cast_expr:
    KW_TO '<' type_specifier '>' '(' expr ')' {
        $$ = ast_new_cast($3, $6);
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
    T_STR { $$ = TYPE_STR; }
    | T_BOOL { $$ = TYPE_BOOL; }
    | T_NONE { $$ = TYPE_VOID; }
    | T_SINT { $$ = TYPE_SINT; }
    | T_UINT { $$ = TYPE_UINT; }
    ;

%%
