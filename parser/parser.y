%{
#include <cstdio>
#include <cstdlib>
#include "lexer.hpp"
%}

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
%token T_string 

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

/* 
 * Associativity is explicitly declared
 * Precedence increases top to bottom 
 *
 * ?? PREFIXES ARE COMMENTED OUT ??
*/

//%left "let" "in"

%left ';'

// if - then - else
%right "then" "else"

%nonassoc ":="

%left "||"
%left "&&"
%nonassoc '=' "<>" '>' '<' "<=" ">=" "==" "!="

%left '+' '-' "+." "-."
%left '*' '/' "*." "/." "mod"
%right "**"

%nonassoc PSIGN "not" "delete"

%nonassoc PFUNCALL

//%left "!"

%nonassoc '[' ']'

%nonassoc "new" 

%%

program: 
    /* nothing */
    |   program definition
;

definition:
    letdef 
    |   typedef
;

letdef:
        "let" def def_list
    |   "let" "rec" def def_list
;

def_list:
        /* nothing */
    |   def_list "and" def 
;

def: 
        T_idlower par_list type_opt '=' expr
    |   "mutable" T_idlower comma_expr_list_opt type_opt
;

par_list:
        /* nothing */
    |   par_list par
;

type_opt:
        /* nothing */
    |   ':' type
;

comma_expr_list_opt:
        /* nothing */
    |   '[' expr comma_expr_list ']'
;

expr_list:
        /* nothing */
    |   expr_list expr 
;

typedef: 
    "type" tdef tdef_list
;

tdef_list:
        /* nothing */
    |   tdef_list "and" tdef 
;

tdef: 
    T_idlower '=' constr constr_list
;

constr_list: 
        /* nothing */
    |   constr_list '|' constr
;

constr:
    T_idupper  type_list_opt
;

type_list_opt:
        /* nothing */
    |   "of" type type_list
;

type_list:
        /* nothing */
    |   type_list type
;

par:
        T_idlower 
    |   '(' T_idlower ':' type ')'
;

type:
        "unit" 
    |   "int" 
    |   "char" 
    |   "bool" 
    |   "float"
    |   '(' type ')' 
    |   type "->" type
    |   type "ref" 
    |   "array" smth_opt "of" type 
    |   T_idlower
;

smth_opt:
        /* nothing */
    |   '[' '*' comma_star_list ']' 
;

comma_star_list:
        /* nothing */
    |   comma_star_list ',' '*'
;

expr: 
        T_intconst
    |   T_floatconst
    |   T_charconst
    |   T_string 
    |   "true"   | "false"
    |   '(' ')'  | '(' expr ')'
    |   unop expr                                   %prec PSIGN 
    |   expr binop expr 
    |   T_idlower expr_list  | T_idupper expr_list // %prec PFUNCALL
    |   T_idlower '[' expr comma_expr_list ']'
    |   "dim" intconst_opt T_idlower
    |   "new" type   | "delete" expr 
    |   letdef "in" expr
    |   "begin" expr "end"
    |   "if" expr "then" expr else_expr_opt 
    |   "while" expr "do" expr "done"
    |   "for" T_idlower '=' expr to_alternatives expr "do" expr "done"
    |   "match" expr "with" clause clause_list "end"
;

comma_expr_list:
        /* nothing */
    |   comma_expr_list ',' expr
;

intconst_opt:
        /* nothing */
    |   T_intconst
;

else_expr_opt:
        /* nothing */
    |   "else" expr
;

to_alternatives:
        "to" 
    |   "downto"
;

clause_list:
        /* nothing */
    |   '|' clause
;

unop: 
        '+' | '-' | "+." | "-." | '!' | "not"
;

binop: 
        '+' | '-' | '*' | '/' | "+." | "-." | "*." | "/." 
    |   "mod" | "**" | '=' | "<>" | '<' | '>' | "<=" | ">=" 
    |   "==" | "!=" | "&&" | "||" | ';' | ":="
;

clause: 
    pattern "->" expr
;

pattern:
        '+' T_intconst %prec PSIGN 
    |   '-' T_intconst %prec PSIGN 
    |   "+." T_floatconst %prec PSIGN 
    |   "-." T_floatconst %prec PSIGN 
    |   T_charconst
    |   "true" | "false"
    |   T_idlower 
    |   '(' pattern ')'
    |   T_idupper pattern_list
;

pattern_list:
        /* nothing */
    |   pattern_list pattern 
;

%%

void yyerror(const char *msg){
    fprintf(stderr, "Error: %s\n", msg);
    exit(1);
}

// Run yyparse in main
int main() {
    int result = yyparse();
    if(result == 0) printf("Success\n");
    return result;
}