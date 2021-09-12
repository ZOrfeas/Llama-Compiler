#include <string>
#include <cstdio>
#include <cstdlib> 

#include "lexer.hpp"
#include "ast.hpp"
#include "parser.hpp"

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

void UnOp::sem() {
        expr->sem();
        TypeGraph *t_expr = expr->get_TypeGraph();

        switch (op)
        {
        case '+':
        case '-':
        {
            expr->type_check(type_int, "Only int allowed");

            TG = type_int;
            break;
        }
        case T_minusdot:
        case T_plusdot:
        {
            expr->type_check(type_float, "Only float allowed");

            TG = type_float;
            break;
        }
        case T_not:
        {
            expr->type_check(type_bool, "Only bool allowed");
            
            TG = type_bool;
            break;
        }
        case '!':
        {
            // Add constraint with ref of unknown type 
            // to ensure that expr is in fact a ref
            TypeGraph *unknown = new UnknownTypeGraph(false, true, false);
            TypeGraph *ref_t = new RefTypeGraph(unknown);
            inf.addConstraint(t_expr, ref_t, line_number);

            TG = ref_t->getContainedType();
            break;
        }
        case T_delete:
        {
            // Adds constraint with ref of unknown type 
            // to ensure that expr is in fact a ref
            TypeGraph *unknown_t = new UnknownTypeGraph(false, true, false);
            TypeGraph *ref_t = new RefTypeGraph(unknown_t);
            inf.addConstraint(t_expr, ref_t, line_number);
            
            TG = type_unit;
            break;
        }
        default:
            break;
        }
}
void BinOp::sem() {
        lhs->sem();
        rhs->sem();

        TypeGraph *t_lhs = lhs->get_TypeGraph();
        TypeGraph *t_rhs = rhs->get_TypeGraph();

        switch (op)
        {
        case '+':
        case '-':
        case '*':
        case '/':
        case T_mod:
        {
            lhs->type_check(type_int, "Only int allowed");
            rhs->type_check(type_int, "Only int allowed");

            TG = type_int;
            break;
        }
        case T_plusdot:
        case T_minusdot:
        case T_stardot:
        case T_slashdot:
        case T_dblstar:
        {
            lhs->type_check(type_float, "Only float allowed");
            rhs->type_check(type_float, "Only float allowed");

            TG = type_float;
            break;
        }
        case T_dblbar:
        case T_dblampersand:
        {
            lhs->type_check(type_bool, "Only bool allowed");
            rhs->type_check(type_bool, "Only bool allowed");

            TG = type_bool;
            break;
        }
        case '=':
        case T_lessgreater:
        case T_dbleq:
        case T_exclameq:
        {
            // Check that they are not arrays or functions
            if (t_lhs->isArray() || t_lhs->isFunction() ||
                t_rhs->isArray() || t_rhs->isFunction())
            {
                printError("Array and Function not allowed");
            }

            // Check that they are of the same type
            same_type(lhs, rhs);

            // The result is bool
            TG = type_bool;
            break;
        }
        case '<':
        case '>':
        case T_leq:
        case T_geq:
        {
            // lhs and rhs can be one of int, char, float
            lhs->checkIntCharFloat();
            rhs->checkIntCharFloat();

            // Check that they are of the same type
            same_type(lhs, rhs);

            // Get the correct type for the result
            TG = type_bool;
            break;
        }
        case T_coloneq:
        {
            // The lhs must be a ref of the same type as the rhs
            RefTypeGraph *correct_lhs = new RefTypeGraph(t_rhs);
            lhs->type_check(correct_lhs, "Must be a ref of " + t_rhs->stringifyType());

            // Cleanup NOTE: If the new TypeGraph is 
            // used for inference it should not be deleted
            // delete correct_lhs;

            // The result is unit
            TG = type_unit;
            break;
        }
        case ';':
        {
            TG = t_rhs; 
        }
        default:
            break;
        }    
}