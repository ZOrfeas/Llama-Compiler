#include <string>
#include <cstdio>
#include <cstdlib> 

#include "lexer.hpp"
#include "ast.hpp"
#include "parser.hpp"

TypeGraph *type_unit = tt.lookupType("unit")->getTypeGraph();
TypeGraph *type_int = tt.lookupType("int")->getTypeGraph();
TypeGraph *type_float = tt.lookupType("float")->getTypeGraph();
TypeGraph *type_bool = tt.lookupType("bool")->getTypeGraph();
TypeGraph *type_char = tt.lookupType("char")->getTypeGraph();

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
            if (!t_expr->isRef())
            {
                printError("Only ref allowed");
            }

            TG = t_expr;
            break;
        }
        case T_delete:
        {
            if (!t_expr->isRef())
            {
                printError("Only ref allowed");
            }
            if (!t_expr->isDynamic())
            {
                printError("Must have been assigned value with new");
            }

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
            TG = tt.lookupType("bool")->getTypeGraph();
            break;
        }
        case '<':
        case '>':
        case T_leq:
        case T_geq:
        {
            std::vector<TypeGraph *> type_char_int_float = { type_char, type_int, type_float };
            lhs->type_check(type_char_int_float, "Only char, int and float allowed");
            rhs->type_check(type_char_int_float, "Only char, int and float allowed");

            // Check that they are of the same type
            same_type(lhs, rhs);

            // Get the correct type for the result
            TG = t_lhs;
            break;
        }
        case T_coloneq:
        {
            // The lhs must be a ref of the same type as the rhs
            RefTypeGraph *correct_lhs = new RefTypeGraph(t_rhs);
            lhs->type_check(correct_lhs);

            // Cleanup
            delete correct_lhs;

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