#include "ast.hpp"

/*
 * Liveness analysis for functions in order to determine 
 * what symbols they access from previous scopes.
 * 
 * Every node is passed the name of the function definition
 * inside which it exists so that it can add external dependencies
 * to the function.
 */

// By default do nothing
void AST::liveness(std::string prevFunc)
{
    return;
}

void Program::liveness(std::string prevFunc)
{
    // Create and insert fake function PROGRAM

    // Recursive call using PROGRAM
    for(auto *d: definition_list) d->liveness("PROGRAM");
}
void Letdef::liveness(std::string prevFunc) 
{
    if (recursive)
    {
        // Insert symbols to table

        // Call liveness on each def 

    }
    else 
    {
        // Call liveness on each def

        // Insert symbols to table

    }
}

insertToTable(std::string id)
{
    // Insert symbol to table

}
void Constant::liveness(std::string prevFunc)
{
    expr->liveness(prevFunc);
}
void Array::liveness(std::string prevFunc)
{
    for(auto *e: expr_list) e->liveness(prevFunc);
}
void Function::liveness(std::string prevFunc) 
{
    // Add this function as a dependency to prevFunc

    // Create new scope
    
    // Insert parameters to table

    // Recursive call to the body giving the name of this function
    expr->liveness(id);

    // Add this function's external symbol dependencies to all
    // functions dependent on it

    // Close scope
}

void BinOp::liveness(std::string prevFunc)
{
    lhs->liveness(prevFunc);
    rhs->liveness(prevFunc);
}
void UnOp::liveness(std::string prevFunc)
{
    expr->liveness(prevFunc);
}

void While::liveness(std::string prevFunc)
{
    cond->liveness(prevFunc);
    body->liveness(prevFunc);
}
void For::liveness(std::string prevFunc)
{
    // Open scope

    // Insert id to table

    // Recursive calls
    start->liveness(prevFunc);
    finish->liveness(prevFunc);
    body->liveness(prevFunc);

    // Close scope

}
void If::liveness(std::string prevFunc)
{
    cond->liveness(prevFunc);
    body->liveness(prevFunc);
    
    if(else_body != nullptr) 
        else_body->liveness(prevFunc);
}

void Dim::liveness(std::string prevFunc)
{
    // Check whether this array belongs to prevFunc's scope

}
void ArrayAccess::liveness(std::string prevFunc)
{
    // Check whether this array belongs to prevFunc's scope

}
void ConstantCall::liveness(std::string prevFunc)
{
    // Check whether this constant belongs to prevFunc's scope

}
void FunctionCall::liveness(std::string prevFunc)
{
    // If called function's definition has been analysed then 
    // add its dependencies to prevFunc

    // If not then prevFunc is dependent on called function

}

// Match stuff just for the scopes