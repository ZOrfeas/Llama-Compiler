%{
#include <ctype.h>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <getopt.h>
#include <iomanip>
#include <vector>

#include "lexer.hpp"
#include "symbol.hpp"
#include "ast.hpp"
#include "infer.hpp"

void compilerHandler(Program *p);

// #define YYERROR_VERBOSE 1 // comment out to disable verbose error report
// #define YYDEBUG 1 // comment out to disable debug feature compilation
%}
/* %define parse.trace */
%define parse.error verbose

%union {
    Par *par;
    Constr *constr;
    DefStmt *def_stmt;
    Definition *definition;
    Program *program;
    Pattern *pat;
    Expr *expr;
    Type *type;
    Clause *clause;
    std::vector<Par *> *par_vect;
    std::vector<Expr *> *expr_vect;
    std::vector<Constr *> *constr_vect;
    //std::vector<Def *> *def_vect;
    //std::vector<Tdef *> *tdef_vect;
    std::vector<DefStmt *> *defstmt_vect;
    std::vector<Type *> *type_vect;
    std::vector<Pattern *> *pat_vect;
    std::vector<Clause *> *clause_vect;
    std::string *id;
    int op;     // This will store the lexical code of the operator
    int num;
    // char character ! No need cause we need the string !
    float dec;
    std::string *str;
    
}

%token T_and "and"
%token T_array "array"
%token T_begin "begin"
%token T_bool "bool"
%token T_char "char"
%token<op> T_delete "delete"
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
%token<op> T_not "not"
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

%token<num> T_intconst 
%token<dec> T_floatconst 
%token<str> T_charconst 
%token<str> T_stringliteral

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
%right "->"

// Operator precedences
%precedence LETIN
%left<op> ';'
%right "then" "else"
%nonassoc<op> ":="
%left<op> "||"
%left<op> "&&"
%nonassoc<op> '=' "<>" '>' '<' "<=" ">=" "==" "!=" COMPOP
%left<op> '+' '-' "+." "-." ADDOP
%left<op> '*' '/' "*." "/." "mod" MULTOP
%right<op> "**"
%precedence UNOPS


%type<program> program program_list
%type<definition> definition_choice letdef typedef
%type<def_stmt> def tdef
%type<par_vect> par_list
%type<expr_vect> bracket_comma_expr_list comma_expr_opt_list /*comma_expr_2_list*/ expr_2_list
%type<pat_vect> pattern_list
%type<constr_vect> bar_constr_opt_list
//%type<def_vect> and_def_opt_list
//%type<tdef_vect> and_tdef_opt_list
%type<defstmt_vect> and_def_opt_list and_tdef_opt_list
%type<constr> constr
%type<type_vect> of_type_opt_list at_least_one_type
%type<par> par
%type<type> type
%type<op> unop comp_operator add_operator mult_operator
%type<expr> expr expr_2
%type<num> comma_star_opt_list bracket_star_opt
%type<pat> pattern 
%type<clause_vect> bar_clause_opt_list
%type<clause> clause

%%
program 
: program_list                      { $$ = $1; compilerHandler($$); } 
;

program_list
: %empty                            { $$ = new Program(); }
| program_list definition_choice    { $1->append($2); $$ = $1; }
;

definition_choice               
: letdef                            { $$ = $1; }
| typedef                           { $$ = $1; }
;

letdef
: "let" def and_def_opt_list        { $3->insert($3->begin(), $2); $$ = new Letdef($3); }
| "let" "rec" def and_def_opt_list  { $4->insert($4->begin(), $3); $$ = new Letdef($4, true); }
;

typedef
: "type" tdef and_tdef_opt_list     { $3->insert($3->begin(), $2); $$ = new Typedef($3); }
;

def
: T_idlower '=' expr                                    { $$ = new Constant($1, $3); }
| T_idlower ':' type '=' expr                           { $$ = new Constant($1, $5, $3); }
| T_idlower par_list '=' expr                           { $$ = new Function($1, $2, $4); }
| T_idlower par_list ':' type '=' expr                  { $$ = new Function($1, $2, $6, $4); }
| "mutable" T_idlower bracket_comma_expr_list           { $$ = new Array($2, $3); }
| "mutable" T_idlower bracket_comma_expr_list ':' type  { $$ = new Array($2, $3, $5); }
| "mutable" T_idlower                                   { $$ = new Variable($2); }
| "mutable" T_idlower ':' type                          { $$ = new Variable($2, $4); }
;

par_list
: par                           { $$ = new std::vector<Par *>(); $$->push_back($1); }
| par_list par                  { $1->push_back($2); $$ = $1; }
;

/*
colon_type_opt
: %empty
| ':' type
;
*/

bracket_comma_expr_list
: '[' expr comma_expr_opt_list ']'  { $3->insert($3->begin(), $2); $$ = $3; }
// : '[' expr ']'                      { $$ = new std::vector<Expr *>(); }
;

comma_expr_opt_list
: %empty                            { $$ = new std::vector<Expr *>(); }
| comma_expr_opt_list ',' expr      { $1->push_back($3); $$ = $1; }
;

tdef
: T_idlower '=' constr bar_constr_opt_list  { $4->insert($4->begin(), $3); $$ = new Tdef($1, $4); }
;

bar_constr_opt_list
: %empty                            { $$ = new std::vector<Constr *>(); }
| bar_constr_opt_list '|' constr    { $1->push_back($3); $$ = $1; }
;

and_def_opt_list
: %empty                            { $$ = new std::vector<DefStmt *>(); }
| and_def_opt_list "and" def        { $1->push_back($3); $$ = $1; }
;

and_tdef_opt_list
: %empty                            { $$ = new std::vector<DefStmt *>(); }
| and_tdef_opt_list "and" tdef      { $1->push_back($3); $$ = $1; }
;

constr
: T_idupper of_type_opt_list        { $$ = new Constr($1, $2); }
;

of_type_opt_list
: %empty                            { $$ = new std::vector<Type *>(); }
| "of" at_least_one_type            { $$ = $2; }
;

at_least_one_type
: type                              { $$ = new std::vector<Type *>(); $$->push_back($1); }
| at_least_one_type type            { $1->push_back($2); $$ = $1; }
;

par
: T_idlower                         { $$ = new Par($1); }
| '(' T_idlower ':' type ')'        { $$ = new Par($2, $4); }
;

type
: "unit"                            { $$ = new BasicType(type::TYPE_unit); }     
| "int"                             { $$ = new BasicType(type::TYPE_int); }
| "char"                            { $$ = new BasicType(type::TYPE_char); }
| "bool"                            { $$ = new BasicType(type::TYPE_bool); }
| "float"                           { $$ = new BasicType(type::TYPE_float); }
| '(' type ')'                      { $$ = $2; }
| type "->" type                    { $$ = new FunctionType($1, $3); }
| type "ref"                        { $$ = new RefType($1); }
| "array" bracket_star_opt "of" type %prec ARRAYOF  { $$ = new ArrayType($2, $4); }
| T_idlower                         { $$ = new CustomType($1); /* LOOKUP CUSTOM TYPE*/; }
;

bracket_star_opt
: %empty                            { $$ = 1; }
| '[' '*' comma_star_opt_list ']'   { $$ = 1 + $3; }
;

comma_star_opt_list
: %empty                            { $$ = 0; }
| comma_star_opt_list ',' '*'       { $$ = 1 + $1; }
;

expr
: letdef "in" expr %prec LETIN          { $$ = new LetIn($1, $3); }
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
| "for" T_idlower '=' expr "to" expr "do" expr "done"       { $$ = new For($2, $4, "to", $6, $8); } 
| "for" T_idlower '=' expr "downto" expr "do" expr "done"   { $$ = new For($2, $4, "downto", $6, $8); }  
| "match" expr "with" clause bar_clause_opt_list "end"      { $5->insert($5->begin(), $4); $$ = new Match($2, $5); }
| "dim" T_intconst T_idlower            { Int_literal *dim = new Int_literal($2); $$ = new Dim($3, dim); }
| "dim" T_idlower                       { $$ = new Dim($2); }
| T_idlower expr_2_list                 { $$ = new FunctionCall($1, $2); /* LOOKUP id */; }
| T_idupper expr_2_list                 { $$ = new ConstructorCall($1, $2); /* LOOKUP Id */ }
| expr_2                                { $$ = $1; }            
;

expr_2
: T_intconst                        { $$ = new Int_literal($1); } 
| T_floatconst                      { $$ = new Float_literal($1); }
| T_charconst                       { $$ = new Char_literal($1); }
| T_stringliteral                   { $$ = new String_literal($1); }
| T_idlower                         { $$ = new ConstantCall($1); /* LOOKUP */ }
| T_idupper                         { $$ = new ConstructorCall($1); /* LOOKUP */ }
| "true"                            { $$ = new Bool_literal(true); }
| "false"                           { $$ = new Bool_literal(false); }
| '(' ')'                           { $$ = new Unit_literal(); }
| '!' expr_2                        { $$ = new UnOp('!', $2); }
| T_idlower bracket_comma_expr_list { $$ = new ArrayAccess($1, $2); /* ARRAY ACCESS */; }
| "new" type                        { $$ = new New($2); /* DYNAMIC ALLOCATION */; }
| '(' expr ')'                      { $$ = $2; }
| "begin" expr "end"                { $$ = $2; }
;

/* comma_expr_2_list
: expr_2                            { $$ = new std::vector<Expr *>(); $$->push_back($1); }
| comma_expr_2_list ',' expr_2      { $1->push_back($3); $$ = $1; }
; */

expr_2_list
: expr_2                        { $$ = new std::vector<Expr *>(); $$->push_back($1); }
| expr_2_list expr_2            { $1->push_back($2); $$ = $1; }
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
: %empty                            { $$ = new std::vector<Clause *>(); }
| bar_clause_opt_list '|' clause    { $1->push_back($3); $$ = $1; }
;

clause
: pattern "->" expr                 { $$ = new Clause($1, $3); }
;

pattern
: '+' T_intconst                        { $$ = new PatternLiteral(new Int_literal($2));     }
| '-' T_intconst                        { $$ = new PatternLiteral(new Int_literal(-$2));    }
| T_intconst                            { $$ = new PatternLiteral(new Int_literal($1));     }
| "+." T_floatconst                     { $$ = new PatternLiteral(new Float_literal($2));   }
| "-." T_floatconst                     { $$ = new PatternLiteral(new Float_literal(-$2));  }
| T_floatconst                          { $$ = new PatternLiteral(new Float_literal($1));    }
| T_charconst                           { $$ = new PatternLiteral(new Char_literal($1));    }
| "true"                                { $$ = new PatternLiteral(new Bool_literal(true));  }
| "false"                               { $$ = new PatternLiteral(new Bool_literal(false)); }
| T_idlower                             { $$ = new PatternId($1);        }
| T_idupper                             { $$ = new PatternConstr($1);     }
| '(' pattern ')'                       { $$ = $2;                          }
|  T_idupper pattern_list         { $$ = new PatternConstr($1, $2); }
;

pattern_list
: pattern                           { $$ = new std::vector< Pattern* >(); $$->push_back($1); }
| pattern_list pattern              { $1->push_back($2); $$ = $1; }
;

%%

bool    syntaxAnalysis, semAnalysis, inferenceAnalysis, compile, 
        tableLogs, inferenceLogs, printTable, printAST, 
        printObjectCode, printFinalCode, 
        printSuccess, help;

enum option_enum 
    {   
        OPTION_tableLogs, OPTION_inferenceLogs, OPTION_printTable, 
        OPTION_printAST, OPTION_printObjectCode, OPTION_printFinalCode,
        OPTION_printSuccess, OPTION_help
    };

std::vector<int> option_values = 
{
    0, 1, 2, 3
};
std::vector<option_enum> long_option_values = 
{
    OPTION_tableLogs, OPTION_inferenceLogs, OPTION_printTable, 
    OPTION_printAST, OPTION_printObjectCode, OPTION_printFinalCode,
    OPTION_printSuccess, OPTION_help
};

std::string option_string[] = 
{
    "s", "a", "i", "c"
};
std::string long_option_string[] = 
{   
    "tableLogs",      
    "inferenceLogs",  
    "printTable",     
    "printAST",       
    "printObjectCode",
    "printFinalCode", 
    "printSuccess",   
    "help",           
};

std::string option_description[] = 
{
    "Checks only if the program is syntactically correct",
    "Checks if the program is also semantically correct",
    "Performs inference to resolve unknown types",
    "Produces code"
};
std::string long_option_description[] = 
{
    "Shows symbol, type and constructor table logs",
    "Shows inference logs", 
    "Prints table with the names of variables and functions and their types",
    "Prints the whole AST produced by the syntactical analysis",
    "Prints object code",
    "Prints final code",
    "Prints success message as opposed to being silent when compilation succeeds",
    "Shows all options and their funcionality"
};

void yyerror(const char *msg) {
    printf("Error at line %d: %s\n", yylineno, msg);
    exit(1);
}

void printHeader(std::string s)
{
    int stars = 20;
    std::cout   << std::setfill('*') << std::setw(stars) << " " << s << " " 
                << std::setfill('*') << std::setw(stars) << " " << std::endl
                << std::endl; 
}

//bool table_logs, inferer_logs;
void compilerHandler(Program *p) {
    if (inferenceLogs) 
    {
        inf.enable_logs();
    }

    if (tableLogs) 
    {
        st.enable_logs();
        tt.enable_logs();
        ct.enable_logs();
    }

    if(syntaxAnalysis)
    {
        if(printAST) 
        {   
            //printHeader("AST");
            std::cout << *p << std::endl; 
            std::cout << std::endl;
        }
    }
    
    if(semAnalysis)
    {
        p->sem(); 
    }

    if(inferenceAnalysis) 
    {
        inf.solveAll(false);
        if(printTable) 
        {   
            //printHeader("Types of identifiers");
            p->printIdTypeGraphs();
            std::cout << std::endl;
        }
    } 

    if(compile) {}
}

int main(int argc, char **argv) {
    syntaxAnalysis = semAnalysis = inferenceAnalysis = compile = 
    tableLogs = inferenceLogs = printTable = 
    printAST = printObjectCode = printFinalCode = 
    printSuccess = help = false;
    
    int option_index = 0;
    struct option long_options[] = {
            {"tableLogs",       no_argument, NULL, OPTION_tableLogs         },
            {"inferenceLogs",   no_argument, NULL, OPTION_inferenceLogs     },
            {"printTable",      no_argument, NULL, OPTION_printTable        },
            {"printAST",        no_argument, NULL, OPTION_printAST          },
            {"printObjectCode", no_argument, NULL, OPTION_printObjectCode   },
            {"printFinalCode",  no_argument, NULL, OPTION_printFinalCode    },
            {"printSuccess",    no_argument, NULL, OPTION_printSuccess      },
            {"help",            no_argument, NULL, OPTION_help              }
        };
    
    int c;
    bool noShortOptions = true; // If there are no short options then run all stages
    while((c = getopt_long(argc, argv, "saic", long_options, &option_index)) != -1)
    {   
        switch(c)
        {
            case OPTION_tableLogs:
                tableLogs = true;
                break;
            case OPTION_inferenceLogs:
                inferenceLogs = true;
                break;
            case OPTION_printTable:
                printTable = true;
                break;
            case OPTION_printAST:
                printAST = true;
                break;
            case OPTION_printObjectCode:
                printObjectCode = true;
                break;
            case OPTION_printFinalCode:
                printFinalCode = true;
                break;
            case OPTION_printSuccess:
                printSuccess = true;
                break;
            case OPTION_help:
                std::cout << "Usage ./llamac -[short options] --[long options] < file" << std::endl;
                std::cout << std::endl;

                for(auto i: option_values)
                {
                    std::cout << std::left << std::setfill(' ') << std::setw(20) << "-" + option_string[i]
                          << std::left << option_description[i]
                          << std::endl;
                }
                for(auto i: long_option_values)
                {
                    std::cout << std::left << std::setfill(' ') << std::setw(20) << "--" + long_option_string[i]
                          << std::left << long_option_description[i]
                          << std::endl;
                }
                exit(0);
                break;
            case 'c':
                compile = true;
            case 'i':
                inferenceAnalysis = true;
            case 'a':
                semAnalysis = true;
            case 's':
                syntaxAnalysis = true;
                noShortOptions = false;
                break;
            default:
                break;
        }
    }

    // If no arguments are given then compile
    if(noShortOptions) 
    {
        syntaxAnalysis = true;
        semAnalysis = true;
        inferenceAnalysis = true;
        compile = true;
    }

    // yydebug = 1; // default val is zero so just comment this to disable
    int result = yyparse();
    
    if (result == 0 && printSuccess) printf("Success\n");
    return result;
}