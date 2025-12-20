%code requires {
    #include <stdint.h>
    #include "ast.h"
    #include "types.h"
}

%{
    #include <stdio.h>
    #include <stdint.h>
    #include <inttypes.h>

    extern int yylineno;

    int yylex();
    void yyerror(const char *msg){ fprintf(stderr, "Error in line %d: %s\n", yylineno, msg); }

    /* Global AST Root */
    ast_node *root = NULL;
%}

%define parse.error verbose

%union {
    uint64_t nr;
    char *str;
    ASTNode *node;
    /* Type info */
    yafltype type;
}

/* Lexer Tokens */
%token <nr> L_INT L_BOOL
%token <str> L_STR ID ID_VAR
%token KW_FN KW_RET KW_IF KW_ELSE KW_WHILE KW_FOR KW_IN KW_RANGE KW_PRINT
%token S_RARROW S_LARROW
%token T_STR T_BOOL T_NONE T_SINT T_UINT

/* Non-terminals */
%type <node> program top_level_list top_level_item
%type <node> fn_definition fn_signature
%type <node> param_list param_list_nonempty param
%type <node> compound_stmt statement_list statement
%type <node> var_decl_stmt assignment_stmt return_stmt
%type <node> if_stmt opt_else for_stmt while_stmt print_stmt expr_stmt
%type <node> for_loop_var
%type <node> expr primary call_expr
%type <node> expr_list expr_list_opt
%type <type> type_specifier

/* Precedence */
%left '<' '>'
%left '+' '-'
%left '*' '/' '%'
%precedence UMINUS  /* Unary minus */

%%

program:
    top_level_list { root = $1; }
    ;

top_level_list:
    top_level_list top_level_item { $$ = ast_append($1, $2); }
    | %empty { $$ = NULL; }
    ;

top_level_item:
    fn_definition
    | compound_stmt
    | statement
    ;

/* --- FUNCTIONS --- */
fn_definition:
    fn_signature compound_stmt {
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
    type_specifier ':' VAR_ID {
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
    | expr_stmt
    ;

/* --- SPECIFIC STATEMENTS --- */
var_decl_stmt:
    type_specifier ID_VAR S_LARROW expr ';' {
        $$ = ast_new_decl($1, $2, $4);
    }
    ;

assignment_stmt:
    ID_VAR S_LARROW expr ';' {
        $$ = ast_new_assign($1, $3);
    }
    ;

return_stmt:
    KW_RET expr ';' {
        $$ = ast_new_return($2);
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
    KW_FOR for_loop_var KW_IN KW_RANGE '(' expr ',' expr ',' expr ')' compound_stmt {
        $$ = ast_new_for($2, $6, $8, $10, $12);
    }
    ;

for_loop_var:
    type_specifier VAR_ID { $$ = $2; }
    | VAR_ID

while_stmt:
    KW_WHILE '(' expr ')' compound_stmt {
        $$ = ast_new_while($3, $5);
    }
    ;

print_stmt:
    KW_PRINT '(' STR_CONST ')' ';' {
        $$ = ast_new_print($3);
    }
    ;

expr_stmt:
    expr ';'
    ;

/* --- expressions --- */
expr:
    expr '+' expr { $$ = ast_new_binary('+', $1, $3); }
    | expr '-' expr { $$ = ast_new_binary('-', $1, $3); }
    | expr '*' expr { $$ = ast_new_binary('*', $1, $3); }
    | expr '/' expr { $$ = ast_new_binary('/', $1, $3); }
    | expr '<' expr { $$ = ast_new_binary('<', $1, $3); }
    | expr '>' expr { $$ = ast_new_binary('>', $1, $3); }
    | '-' expr %prec AR_UMINUS { $$ = ast_new_unary('-', $2); }
    | '(' expr ')' { $$ = $2; }
    | primary { $$ = $1; }
    ;

primary:
    L_INT { $$ = ast_new_int($1); }
    | L_STR { $$ = ast_new_str($1); }
    | L_CHR { $$ = ast_new_str($1); }
    | L_BOOL { $$ = ast_new_str($1); }
    | ID_VAR { $$ = ast_new_var($1); }
    | call_expr { $$ = $1; }
    ;

call_expr:
    IDENTIFIER '(' expr_list_opt ')' {
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
    T_STR { $$ = TYPE_STRING; }
    | T_CHAR { $$ = TYPE_CHAR; }
    | T_BOOL { $$ = TYPE_BOOL; }
    | T_NONE { $$ = TYPE_VOID; }
    | T_SINT8 { $$ = TYPE_SINT8; }
    | T_SINT16 { $$ = TYPE_SINT16; }
    | T_SINT32 { $$ = TYPE_SINT32; }
    | T_SINT64 { $$ = TYPE_SINT64; }
    | T_UINT8 { $$ = TYPE_UINT8; }
    | T_UINT16 { $$ = TYPE_UINT16; }
    | T_UINT32 { $$ = TYPE_UINT32; }
    | T_UINT64 { $$ = TYPE_UINT64; }
    ;

%%

int main(){
    yyparse();
}
