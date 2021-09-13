#include <string>
#include <cstdio>
#include <cstdlib> 

#include "ast.hpp"

std::vector<Identifier *> AST_identifier_list = {};

TypeGraph *type_unit = tt.lookupType("unit")->getTypeGraph();
TypeGraph *type_int = tt.lookupType("int")->getTypeGraph();
TypeGraph *type_float = tt.lookupType("float")->getTypeGraph();
TypeGraph *type_bool = tt.lookupType("bool")->getTypeGraph();
TypeGraph *type_char = tt.lookupType("char")->getTypeGraph();

void printColorString(std::string s, int width, int format, int color)
{   
    std::string intro = (format == 0) ? "\033[0m" 
                                      : "\033[" + std::to_string(format) + ";" + std::to_string(color) + "m";
    std::string outro = "\033[0m";
    
    std::cout   << intro
                << std::left << std::setfill(' ') << std::setw(width)
                << s 
                << outro; 
}
