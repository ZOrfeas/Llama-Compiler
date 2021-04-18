%{
#include <cstdio>
#include <cstdlib>
#include "lexer.hpp"
#define YYDEBUG 1 // comment out to disable debug feature compilation
%}
/* %define parse.trace */

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

%token T_idlower 
%token T_idupper 

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
%left ';'
%right "then" "else"
%nonassoc ":="
%left "||"
%left "&&"
%nonassoc '=' "<>" '>' '<' "<=" ">=" "==" "!=" COMPOP
%left '+' '-' "+." "-." ADDOP
%left '*' '/' "*." "/." "mod" MULTOP
%right "**"
%precedence UNOPS

%%

program
: %empty
| program definition_choice
;

definition_choice
: letdef | typedef
;

letdef
: "let" def and_def_opt_list
| "let" "rec" def and_def_opt_list
;

typedef
: "type" tdef and_tdef_opt_list
;

def
: T_idlower par_opt_list colon_type_opt '=' expr
| "mutable" T_idlower bracket_comma_expr_opt colon_type_opt
;

par_opt_list
: %empty
| par_opt_list par
;

colon_type_opt
: %empty
| ':' type
;

bracket_comma_expr_opt
: %empty
| '[' expr comma_expr_opt_list ']'
;

comma_expr_opt_list
: %empty
| comma_expr_opt_list ',' expr
;

tdef
: T_idlower '=' constr bar_constr_opt_list
;

bar_constr_opt_list
: %empty
| bar_constr_opt_list '|' constr
;

and_def_opt_list
: %empty
| and_def_opt_list "and" def
;

and_tdef_opt_list
: %empty
| and_tdef_opt_list "and" tdef
;

constr
: T_idupper of_type_opt_list
;

of_type_opt_list
: %empty
| "of" at_least_one_type
;

at_least_one_type
: type
| at_least_one_type type
;

par
: T_idlower
| '(' T_idlower ':' type ')'
;

type
: "unit" | "int" | "char" | "bool" | "float"
| '(' type ')' 
| type "->" type 
| type "ref"
| "array" bracket_star_opt "of" type %prec ARRAYOF
| T_idlower
;

bracket_star_opt
: %empty
| '[' '*' comma_star_opt_list ']'
;

comma_star_opt_list
: %empty
| comma_star_opt_list ',' '*'
;

expr
: letdef "in" expr %prec LETIN
| expr ';' expr
| "if" expr "then" expr "else" expr 
| "if" expr "then" expr
| expr ":=" expr
| expr "||" expr
| expr "&&" expr
// remember to sometime check if you can group them before doing the ASTs
//| expr '=' expr | expr "<>" expr | expr '>' expr | expr '<' expr | expr "<=" expr | expr ">=" expr | expr "==" expr | expr "!=" expr 
| expr comp_operator expr %prec COMPOP
//| expr '+' expr | expr '-' expr | expr "+." expr | expr "-." expr 
| expr add_operator expr %prec ADDOP
/* | expr '*' expr | expr '/' expr | expr "*." expr | expr "/." expr | expr "mod" expr  */
| expr mult_operator expr %prec MULTOP
| expr "**" expr
| unop expr %prec UNOPS
| "while" expr "do" expr "done"
| "for" T_idlower '=' expr to_or_downto expr "do" expr "done"
| "match" expr "with" clause bar_clause_opt_list "end"
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
: '+' | '-' | "+." | "-." | "not" | "delete"
;

comp_operator
: '=' | "<>" | '>' | '<' | "<=" | ">=" | "==" | "!="
;

add_operator
: '+' | '-' | "+." | "-."
;

mult_operator
: '*' | '/' | "*." | "/." | "mod"
;

to_or_downto
: "to" | "downto"
;

bar_clause_opt_list
: %empty
| bar_clause_opt_list '|' clause
;

clause
: pattern "->" expr
;

pattern
: '+' T_intconst | '-' T_intconst
| "+." T_floatconst | "-." T_floatconst
| T_charconst 
| "true" | "false"
| T_idlower
| '(' pattern ')'
| '(' T_idupper pattern_opt_list ')'
;

pattern_opt_list
: %empty
| pattern_opt_list pattern
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