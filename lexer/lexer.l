%{
// token definitions, possibly omissible later on
#define T_eof 0
#define T_and 1000
#define T_array 1001
#define T_begin 1002
#define T_bool 1003
#define T_char 1004
#define T_delete 1005
#define T_dim 1006
#define T_do 1007
#define T_done 1008
#define T_downto 1009
#define T_else 1010
#define T_end 1011
#define T_false 1012
#define T_float 1013
#define T_for 1014
#define T_if 1015
#define T_in 1016
#define T_int 1017
#define T_let 1018
#define T_match 1019
#define T_mod 1020
#define T_mutable 1021
#define T_new 1022
#define T_not 1023
#define T_of 1024
#define T_rec 1025
#define T_ref 1026
#define T_then 1027
#define T_to 1028
#define T_true 1029
#define T_type 1030
#define T_unit 1031
#define T_while 1032
#define T_with 1033

#define T_idlower 1034
#define T_idupper 1035

#define T_intconst 1036
#define T_floatconst 1037
#define T_charconst 1038
#define T_string 1039

#define T_dashgreater 1040
#define T_plusdot 1041
#define T_minusdot 1042
#define T_stardot 1043
#define T_slashdot 1044
#define T_dblstar 1045
#define T_dblampersand 1046
#define T_dblbar 1047
#define T_lessgreater 1048
#define T_leq 1049
#define T_geq 1050
#define T_dbleq 1051
#define T_exclameq 1052
#define T_coloneq 1053

#include <cstdio>
#include <cstdlib>

// int yylineno = 1;
int comment_cnt = 0;
void lex_error(const char *msg);
%}

W [ \t\n\r]  
D  [0-9]     
Ll [a-z]     
Lu [A-Z]     
L  [A-Za-z]
Hex [0-9a-f] 

%option noyywrap

/* use variable yylineno to get current input line number */
%option yylineno 
%option nounput

%x IN_COMMENT

%%

"and"       { return T_and;     }
"array"     { return T_array;   }
"begin"     { return T_begin;   }
"bool"      { return T_bool;    }
"char"      { return T_char;    }
"delete"    { return T_delete;  }
"dim"       { return T_dim;     }
"do"        { return T_do;      }
"done"      { return T_done;    }
"downto"    { return T_downto;  }
"else"      { return T_else;    }
"end"       { return T_end;     }
"false"     { return T_false;   }
"float"     { return T_float;   }
"for"       { return T_for;     }
"if"        { return T_if;      }
"in"        { return T_in;      }
"int"       { return T_int;     }
"let"       { return T_let;     }
"match"     { return T_match;   }
"mod"       { return T_mod;     }
"mutable"   { return T_mutable; }
"new"       { return T_new;     }
"not"       { return T_not;     }
"of"        { return T_of;      }
"rec"       { return T_rec;     }
"ref"       { return T_ref;     }
"then"      { return T_then;    }
"to"        { return T_to;      }
"true"      { return T_true;    }
"type"      { return T_type;    }
"unit"      { return T_unit;    }
"while"     { return T_while;   }
"with"      { return T_with;    }

{Ll}({L}|{D}|_)*    { return T_idlower; }
{Lu}({L}|{D}|_)*    { return T_idupper; }

{D}+                        { return T_intconst;   }
({D}+\.{D}+(e[+\-]?{D}+)?)   { return T_floatconst; }

\'([^'"\\\n\r\0]|(\\([ntr0\\'"]|(x{Hex}{Hex}))))\'    { return T_charconst; }
\"((\\.|[^"\\\n])?)*\"    { return T_string; }

[=\|+\-\*/!;<>]    { return yytext[0]; }
[\(\)\[\],:]        { return yytext[0]; }

"->" { return T_dashgreater;  }
"+." { return T_plusdot;      }
"-." { return T_minusdot;     }
"*." { return T_stardot;      }
"/." { return T_slashdot;     }
"**" { return T_dblstar;      }
"&&" { return T_dblampersand; }
"||" { return T_dblbar;       }
"<>" { return T_lessgreater;  }
"<=" { return T_leq;          }
">=" { return T_geq;          }
"==" { return T_dbleq;        }
"!=" { return T_exclameq;      }
":=" { return T_coloneq;      }

--.*\n  { /* nothing */ }

"(*"                        { comment_cnt++; BEGIN(IN_COMMENT); /*printf("comment_cnt is %d",comment_cnt);*/}
<IN_COMMENT>"(*"            { comment_cnt++; /*printf("comment_cnt is %d",comment_cnt);*/}
<IN_COMMENT>"*)"            { comment_cnt--; if (comment_cnt==0) {/*printf("comment_cnt is %d",comment_cnt);*/BEGIN(INITIAL);} }    
<IN_COMMENT>"*"             { /* nothing */ }
<IN_COMMENT>[^(*<<EOF>>]*   { /* nothing */ }
<IN_COMMENT>"("             { /* nothing */ }
<IN_COMMENT>.               { lex_error("EOF file in comment"); }

{W}+                { /* nothing */ } 

.   { lex_error("Illegal character"); }

%%

void lex_error(const char *msg) {
    fprintf(stderr, "Error at line %d, token %c (ASCII: %d): %s  \n",
        yylineno, yytext[0], yytext[0], msg);
    exit(1);
}

int main() {
    int token;
    do {
        token = yylex();
        printf("token=%d, lexeme=\"%s\"\n", token, yytext);
    } while (token != T_eof);
}
