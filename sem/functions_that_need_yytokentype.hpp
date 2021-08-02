#pragma once

#include <string>
#include <cstdio>
#include <cstdlib> 

#include "lexer.hpp"
#include "ast.hpp"
#include "parser.hpp"

void UnOp::sem() {
        expr->sem();
        TypeGraph *t_expr = expr->get_TypeGraph();

        switch (op)
        {
        case '+':
        case '-':
        {
            if (!t_expr->isInt())
            {
                printError("Only int allowed");
            }
            TG = tt.lookupType("int")->getTypeGraph();
            break;
        }
        case T_minusdot:
        case T_plusdot:
        {
            if (!t_expr->isFloat())
            {
                printError("Only float allowed");
            }
            TG = tt.lookupType("float")->getTypeGraph();
            break;
        }
        case T_not:
        {
            if (!t_expr->isBool())
            {
                printError("Only bool allowed");
            }
            TG = tt.lookupType("bool")->getTypeGraph();
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

            TG = tt.lookupType("unit")->getTypeGraph();
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
            if (!t_lhs->isInt() || !t_rhs->isInt())
            {
                printError("Only int allowed");
            }

            TG = tt.lookupType("int")->getTypeGraph();
            break;
        }
        case T_plusdot:
        case T_minusdot:
        case T_stardot:
        case T_slashdot:
        case T_dblstar:
        {
            if (!t_lhs->isFloat() || !t_rhs->isFloat())
            {
                printError("Only float allowed");
            }

            TG = tt.lookupType("float")->getTypeGraph();
            break;
        }
        case T_dblbar:
        case T_dblampersand:
        {
            if (!t_lhs->isBool() || !t_rhs->isBool())
            {
                printError("Only bool allowed");
            }

            TG = tt.lookupType("bool")->getTypeGraph();
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
            // Check that they are char, int or float
            if (!t_lhs->isChar() && !t_lhs->isInt() && !t_lhs->isFloat() &&
                !t_rhs->isChar() && !t_rhs->isInt() && !t_rhs->isFloat())
            {
                printError("Only char, int and float allowed");
            }

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
            TG = tt.lookupType("unit")->getTypeGraph();
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