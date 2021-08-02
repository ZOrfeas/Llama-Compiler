%{
// token definitions, possibly omissible later on
#define T_eof 0

#include <string>
#include <cstdio>
#include <cstdlib> 

#include "lexer.hpp"
#include "ast.hpp"      // Doesn't include parser.hpp
#include "parser.hpp"   

// ast.cpp is linked and it brings parser.hpp and the functions that need yytokentype

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
"delete"    { yylval.op = T_delete; return T_delete;  }
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
"mod"       { yylval.op = T_mod; return T_mod;     }
"mutable"   { return T_mutable; }
"new"       { return T_new;     }
"not"       { yylval.op = T_not; return T_not;     }
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

{Ll}({L}|{D}|_)*    { yylval.id = new std::string(yytext); return T_idlower; }
{Lu}({L}|{D}|_)*    { yylval.id = new std::string(yytext); return T_idupper; }

{D}+                        { yylval.num = atoi(yytext); return T_intconst;   }
({D}+\.{D}+(e[+\-]?{D}+)?)   { return T_floatconst; }

\'([^'"\\\n\r\0]|(\\([ntr0\\'"]|(x{Hex}{Hex}))))\'    { yylval.str = new std::string(yytext); return T_charconst; }
\"((\\.|[^"\\\n])?)*\"    { yylval.str = new std::string(yytext); return T_stringliteral; }

[=\|+\-\*/!;<>]    { yylval.op = yytext[0]; return yytext[0]; }
[\(\)\[\],:]        { yylval.op = yytext[0]; return yytext[0]; }

"->" { yylval.op = T_dashgreater; return T_dashgreater;  }
"+." { yylval.op = T_plusdot; return T_plusdot;      }
"-." { yylval.op = T_minusdot;     return T_minusdot;     }
"*." { yylval.op = T_stardot;      return T_stardot;      }
"/." { yylval.op = T_slashdot;     return T_slashdot;     }
"**" { yylval.op = T_dblstar;      return T_dblstar;      }
"&&" { yylval.op = T_dblampersand; return T_dblampersand; }
"||" { yylval.op = T_dblbar;       return T_dblbar;       }
"<>" { yylval.op = T_lessgreater;  return T_lessgreater;  }
"<=" { yylval.op = T_leq;          return T_leq;          }
">=" { yylval.op = T_geq;          return T_geq;          }
"==" { yylval.op = T_dbleq;        return T_dbleq;        }
"!=" { yylval.op = T_exclameq;     return T_exclameq;     }
":=" { yylval.op = T_coloneq;      return T_coloneq;      }

--.*\n  { /* nothing */ }

"(*"                        { comment_cnt++; BEGIN(IN_COMMENT); /*printf("comment_cnt is %d",comment_cnt);*/}
<IN_COMMENT>"(*"            { comment_cnt++; /*printf("comment_cnt is %d",comment_cnt);*/}
<IN_COMMENT>"*)"            { comment_cnt--; if (comment_cnt==0) {/*printf("comment_cnt is %d",comment_cnt);*/BEGIN(INITIAL);} }    
<IN_COMMENT>"*"             { /* nothing */ }
<IN_COMMENT>[^(*<<EOF>>]*   { /* nothing */ }
<IN_COMMENT>"("             { /* nothing */ }
<IN_COMMENT>.               { yyerror("EOF file in comment"); }

{W}+    { /* nothing */ } 

. { yyerror("Illegal character"); }

%%

/*
void lex_error(const char *msg) {
    fprintf(stderr, "Error at line %d, token %c (ASCII: %d): %s  \n",
        yylineno, yytext[0], yytext[0], msg);
    exit(1);
}
*/
