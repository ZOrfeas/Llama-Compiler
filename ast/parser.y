%{
#include <cstdio>
#include <cstdlib>
#include "lexer.hpp"
#include "ast.hpp"
//#define YYDEBUG 1 // comment out to disable debug feature compilation
%}
/* %define parse.trace */

%union {
    Par *par;
    Constr *constr;
    DefStmt *def_stmt;
    Definition *definition;
    Program *program;
    Expr *expr;
    Type type;
    std::vector<Par *> par_vect;
    std::vector<Expr *> expr_vect;
    std::vector<Constr *> constr_vect;
    std::vector<Def *> def_vect;
    std::vector<Tdef *> tdef_vect;
    std::vector<Type> type_vect;
    std::string id;
    int op;     // This will store the lexical code of the operator
    int num;
}

%token T_and "and"
%token T_array "array"
%token T_begin "begin"
%token T_bool "bool"
%token T_char "char"
%token T_delete "delete"
%token T_dim "dim"
%token T_do "do"
%token T_done "done"
%token T_downto "downto"
%token T_else "else"
%token T_end "end"
%token T_false "false"
%token T_float "float"
%token T_for "for"
%token T_if "if"
%token T_in "in"
%token T_int "int"
%token T_let "let"
%token T_match "match"
%token T_mod "mod"
%token T_mutable "mutable"
%token T_new "new"
%token T_not "not"
%token T_of "of"
%token T_rec "rec"
%token T_ref "ref"
%token T_then "then"
%token T_to "to"
%token T_true "true"
%token T_type "type"
%token T_unit "unit"
%token T_while "while"
%token T_with "with"

%token<id> T_idlower 
%token<id> T_idupper 

%token T_intconst 
%token T_floatconst 
%token T_charconst 
%token T_stringliteral

%token T_dashgreater "->"
%token T_plusdot "+."
%token T_minusdot "-."
%token T_stardot "*."
%token T_slashdot "/."
%token T_dblstar "**"
%token T_dblampersand "&&"
%token T_dblbar "||"
%token T_lessgreater "<>"
%token T_leq "<="
%token T_geq ">="
%token T_dbleq "=="
%token T_exclameq "!="
%token T_coloneq ":="

/**
 * Associativity and precedence information 
 */

// Type definition necessary precedences
%precedence ARRAYOF
%precedence "ref"
%left "->"

// Operator precedences
%precedence LETIN
%left<op> ';'
%right<op> "then" "else"
%nonassoc<op> ":="
%left<op> "||"
%left<op> "&&"
%nonassoc<op> '=' "<>" '>' '<' "<=" ">=" "==" "!=" COMPOP
%left<op> '+' '-' "+." "-." ADDOP
%left<op> '*' '/' "*." "/." "mod" MULTOP
%right<op> "**"
%precedence UNOPS


%type<program> program
%type<definition> definition_choice letdef typedef
%type<def_stmt> def tdef
%type<par_vect> par_opt_list
%type<expr_vect> bracket_comma_expr_opt comma_expr_opt_list 
%type<constr_vect> bar_constr_opt_list
%type<def_vect> and_def_opt_list
%type<tdef_vect> and_tdef_opt_list
%type<constr> constr
%type<type_vect> of_type_opt_list at_least_one_type
%type<par> par
%type<type> type
%type<op> unop comp_operator add_operator mult_operator
%type<expr> expr
%type<num> comma_star_opt_list bracket_star_opt


%%

program
: %empty                        { $$ = new Program(); }
| program definition_choice     { $1->append($2); $$ = $1; }
;

definition_choice               
: letdef                        { $$ = $1; }
| typedef                       { $$ = $1; }
;

letdef
: "let" def and_def_opt_list        { $3->push_back($2); $$ = new Letdef($3); }
| "let" "rec" def and_def_opt_list  { $4->push_back($3); $$ = new Letdef($4, true); }
;

typedef
: "type" tdef and_tdef_opt_list     { $3->push_back($2); $$ = new Typedef($3); }
;

def
: T_idlower par_opt_list '=' expr                       { $$ = new Function($1, $2, $3); }
| T_idlower par_opt_list ':' type '=' expr              { $$ = new Function($1, $2, $6, $4); }
| "mutable" T_idlower bracket_comma_expr_opt            { $$ = new Mutable($2, $3); }
| "mutable" T_idlower bracket_comma_expr_opt ':' type   { $$ = new Mutable($2, $3, $4); }
;

par_opt_list
: %empty            { $$ = std::vector<Par *>(); }
| par_opt_list par  { $1->push_back($2); $$ = $1; }
;

/*
colon_type_opt
: %empty
| ':' type
;
*/

bracket_comma_expr_opt
: %empty                            { $$ = std::vector<Expr *>(); }
| '[' expr comma_expr_opt_list ']'  { $3->push_back($2); $$ = $3; }
;

comma_expr_opt_list
: %empty                            { $$ = std::vector<Expr *>(); }
| comma_expr_opt_list ',' expr      { $1->push_back($3); $$ = $1; }
;

tdef
: T_idlower '=' constr bar_constr_opt_list  { $$ = new Tdef($1, $3); }
;

bar_constr_opt_list
: %empty                            { $$ = std::vector<Constr *>(); }
| bar_constr_opt_list '|' constr    { $1->push_back($3); $$ = $1; }
;

and_def_opt_list
: %empty                            { $$ = std::vector<Def *>(); }
| and_def_opt_list "and" def        { $1->push_back($3); $$ = $1; }
;

and_tdef_opt_list
: %empty                            { $$ = std::vector<Tdef *>(); }
| and_tdef_opt_list "and" tdef      { $1->push_back($3); $$ = $1; }
;

constr
: T_idupper of_type_opt_list        { $$ = new Constr($1, $2); }
;

of_type_opt_list
: %empty                            { $$ = std::vector<Type>(); }
| "of" at_least_one_type            { $$ = $2; }
;

at_least_one_type
: type                              { $$ = std::vector<Type>(); $$->push_back($1); }
| at_least_one_type type            { $1->push_back($2); $$ = $1; }
;

par
: T_idlower                         { $$ = new Par($1); }
| '(' T_idlower ':' type ')'        { $$ = new Par($2, $4); }
;

type
: "unit"            { $$ = TYPE_unit; }     
| "int"             { $$ = TYPE_int; }
| "char"            { $$ = TYPE_char; }
| "bool"            { $$ = TYPE_bool; }
| "float"           { $$ = TYPE_float; }
| '(' type ')'      { $$ = $2; }
| type "->" type    { ; }
| type "ref"        { ; }
| "array" bracket_star_opt "of" type %prec ARRAYOF  { ; }
| T_idlower         { ; }
;

bracket_star_opt
: %empty                            { $$ = 0; }
| '[' '*' comma_star_opt_list ']'   { $$ = 1 + $3; }
;

comma_star_opt_list
: %empty                            { $$ = 0; }
| comma_star_opt_list ',' '*'       { $$ = 1 + $1; }
;

expr
: letdef "in" expr %prec LETIN          { ; }
| expr ';' expr                         { $$ = new BinOp($1, $2, $3); }
| "if" expr "then" expr "else" expr     { $$ = new If($2, $4, $6); }
| "if" expr "then" expr                 { $$ = new If($2, $4); }
| expr ":=" expr                        { $$ = new BinOp($1, $2, $3); }
| expr "||" expr                        { $$ = new BinOp($1, $2, $3); }
| expr "&&" expr                        { $$ = new BinOp($1, $2, $3); }
// remember to sometime check if you can group them before doing the ASTs
//| expr '=' expr | expr "<>" expr | expr '>' expr | expr '<' expr | expr "<=" expr | expr ">=" expr | expr "==" expr | expr "!=" expr 
| expr comp_operator expr %prec COMPOP  { $$ = new BinOp($1, $2, $3); }
//| expr '+' expr | expr '-' expr | expr "+." expr | expr "-." expr 
| expr add_operator expr %prec ADDOP    { $$ = new BinOp($1, $2, $3); }
/* | expr '*' expr | expr '/' expr | expr "*." expr | expr "/." expr | expr "mod" expr  */
| expr mult_operator expr %prec MULTOP  { $$ = new BinOp($1, $2, $3); }
| expr "**" expr                        { $$ = new BinOp($1, $2, $3); }
| unop expr %prec UNOPS                 { $$ = new UnOp($1, $2); }
| "while" expr "do" expr "done"         { $$ = new While($2, $4); }
| "for" T_idlower '=' expr "to" expr "do" expr "done"   { $$ = new For($2, $4, $5, $6, $8); } 
| "for" T_idlower '=' expr "downto" expr "do" expr "done"   { $$ = new For($2, $4, $5, $6, $8); }  
| "match" expr "with" clause bar_clause_opt_list "end"  { ; }
| "dim" T_intconst T_idlower | "dim" T_idlower
| T_idlower expr_2_opt_list
| T_idupper expr_2_opt_list
| expr_2
;

expr_2
: T_intconst | T_floatconst | T_charconst | T_stringliteral
| T_idlower | T_idupper
| "true" | "false" | '(' ')'
| '!' expr_2
| T_idlower '[' expr_2 comma_expr_2_opt_list ']'
| "new" type
| '(' expr ')'
| "begin" expr "end"
;

comma_expr_2_opt_list
: %empty
| comma_expr_2_opt_list ',' expr_2
;

expr_2_opt_list
: expr_2
| expr_2_opt_list expr_2
;

unop
: '+'       { $$ = $1; }
| '-'       { $$ = $1; }
| "+."      { $$ = $1; }
| "-."      { $$ = $1; }
| "not"     { $$ = $1; }
| "delete"  { $$ = $1; }
;

comp_operator
: '='       { $$ = $1; }
| "<>"      { $$ = $1; }
| '>'       { $$ = $1; }
| '<'       { $$ = $1; }
| "<="      { $$ = $1; }
| ">="      { $$ = $1; }
| "=="      { $$ = $1; }
| "!="      { $$ = $1; }
;

add_operator
: '+'       { $$ = $1; }
| '-'       { $$ = $1; }
| "+."      { $$ = $1; }
| "-."      { $$ = $1; }
;

mult_operator
: '*'       { $$ = $1; }
| '/'       { $$ = $1; }
| "*."      { $$ = $1; }
| "/."      { $$ = $1; }
| "mod"     { $$ = $1; }
;

/*
to_or_downto
: "to"      { $$ = $1; }
| "downto"  { $$ = $1; }
;
*/

bar_clause_opt_list
: %empty                            { ; }
| bar_clause_opt_list '|' clause    { ; }
;

clause
: pattern "->" expr                 { ; }
;

pattern
: '+' T_intconst                    { ; }
| '-' T_intconst                    { ; }
| "+." T_floatconst                 { ; }
| "-." T_floatconst                 { ; }
| T_charconst                       { ; }
| "true"                            { ; }
| "false"                           { ; }
| T_idlower                         { ; }
| '(' pattern ')'                   { ; }
| '(' T_idupper pattern_opt_list ')'    { ; }
;

pattern_opt_list
: %empty                            { ; }
| pattern_opt_list pattern          { ; }
;

%%

void yyerror(const char *msg) {
    fprintf(stderr, "Error at line %d: %s\n", yylineno, msg);
    exit(1);
}

int main() {
    // yydebug = 1; // default val is zero so just comment this to disable
    int result = yyparse();
    if (result == 0) printf("Success\n");
    return result;
}