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

%left PSIGN //"not" "delete"

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
|   "let" rec def def_list
;

def_list:
    /* nothing */
|   def_list "and" def 
;

def: 
    T_idlower par_list type_opt '=' expr
|   "mutable" id expr_list_opt type_opt
;

type_opt:
    /* nothing */
|   ':' type
;

epr_list_opt:
    /* nothing */
|   '[' expr expr_list ']'
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
|   comma_star_list "," "*"
;

// ?? EXPR ??

%%

// Run yyparse in main