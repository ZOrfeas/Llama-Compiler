#include "ast.hpp"

/*
 * Liveness analysis for functions in order to determine 
 * what symbols they access from previous scopes.
 * 
 * Every node is passed the name of the function definition
 * inside which it exists so that it can add external dependencies
 * to the function.
 */

class LivenessEntry
{
protected:
    int scope;
    std::string id;
    TypeGraph *typegraph;

public:
    LivenessEntry(int scope, std::string id, TypeGraph *typegraph)
        : scope(scope), id(id), typegraph(typegraph) {}
    TypeGraph *getTypeGraph() 
    {
        return typegraph;
    }
    std::string getId()
    {
        return id;
    }
    int getScope()
    {
        return scope;
    }
    
    virtual void addDependent(LivenessFunctionEntry *f)
    {}
    virtual void addExternal(LivenessEntry *l) 
    {}
    virtual std::vector<LivenessFunctionEntry *> getDependent()
    {}
    virtual std::vector<LivenessEntry *> getExternal()
    {}
};
class LivenessFunctionEntry
    : public LivenessEntry
{
protected:
    std::vector<LivenessFunctionEntry *> dependent = {};
    std::vector <LivenessEntry *> external = {};

public:
    LivenessFunctionEntry(int scope, std::string id, TypeGraph *typegraph)
        : LivenessEntry(scope, id, typegraph) {}
    virtual void addDependent(LivenessFunctionEntry *f) override 
    {
        dependent.push_back(f);
    }
    virtual void addExternal(LivenessEntry *l) override 
    {
        external.push_back(l);
    }
    virtual std::vector<LivenessFunctionEntry *> getDependent() override
    {
        return dependent;
    }
    virtual std::vector<LivenessEntry *> getExternal() override 
    {
        return external;
    }
};

/** @param T is the Type the table will will hold pointers of */
template <class T>
class LivenessTable
{
    std::vector<std::map<std::string, T> *> *table;
    bool nameInScope(std::string name, std::map<std::string, T> *scope)
    {
        return (scope->find(name) != scope->end());
    }

public:
    LivenessTable() : table(new std::vector<std::map<std::string, T> *>())
    {
        openScope(); // open first (global) scope
    }
    /** Can be called with the implicit pair constructor
     * e.g.: insert({"a_name", a_pointer})
    */
    void insert(std::pair<std::string, T> entry)
    {
        (*table->back())[entry.first] = entry.second;
    }
    T operator[](std::string name)
    {
        for (auto it = table->rbegin(); it != table->rend(); it++)
        {
            if (nameInScope(name, *it))
                return (**it)[name];
        }
        return nullptr;
    }
    void openScope()
    {
        table->push_back(new std::map<std::string, T>());
    }
    void closeScope()
    {
        if (table->size() != 0)
        {
            std::map<std::string, T> *poppedScope = table->back();
            table->pop_back();
            delete poppedScope;
        }
    }
    int getCurrScope()
    {
        return (table->size() - 1);
    }
    ~LivenessTable() {}
};

LivenessTable<LivenessEntry *> LTable;

int getScopeOf(std::string name)
{
    LivenessEntry *l = LTable[name];
    return l->getScope();
}
int isInScopeOf(std::string id, std::string func)
{
    return (getScopeOf(func) < getScopeOf(id));
}
void openScope()
{
    LTable.openScope();
}
void closeScope()
{
    LTable.closeScope();
}
void insertFunctionToLTable(std::string name, TypeGraph *t)
{
    int scope = LTable.getCurrScope();
    LTable.insert({name, new LivenessFunctionEntry(scope, name, t)});
}
void insertSymbolToLTable(std::string name, TypeGraph *t)
{
    int scope = LTable.getCurrScope();
    LTable.insert({name, new LivenessEntry(scope, name, t)});
}
LivenessEntry *lookupLTable(std::string name)
{
    return LTable[name];
}

/*******************************************************/
const std::string progFunc = "PROGRAM";

// By default do nothing
void AST::liveness(std::string prevFunc)
{
    return;
}

void Program::liveness(std::string prevFunc)
{
    // Create and insert fake function
    insertSymbolToLTable(progFunc, nullptr);

    // Recursive call using it
    for (auto *d : definition_list)
        d->liveness(progFunc);
}
void Letdef::liveness(std::string prevFunc)
{
    if (recursive)
    {
        // Insert functions to table
        for(auto *d: def_list)
        {
            insertFunctionToLTable(d->getId(), d->getTypeGraph());        
        }

        // Call liveness on each def
        for(auto *d: def_list)
            d->liveness(prevFunc);
    }
    else
    {
        // Call liveness on each def
        for(auto *d: def_list)
            d->liveness(prevFunc);

        // Insert symbols to table
        for(auto *d: def_list)
        {
            if(d->isFunctionDefinition())
                insertFunctionToLTable(d->getId(), d->getTypeGraph());        
            else 
                insertSymbolToLTable(d->getId(), d->getTypeGraph());
        }
    }

    // Carry over results to the vectors in Functions
    for(auto *d: def_list)
    {
        if(d->isFunctionDefinition())
        {
            auto *f = dynamic_cast<Function *>(d);
            auto ext_list = lookupLTable(f->getId())->getExternal();
            
            for(auto *ext: ext_list)
            {
                f->addExternal(ext->getTypeGraph());
            }
        }
    }
}

TypeGraph *Def::getTypeGraph()
{
    return TG;
}
void Constant::liveness(std::string prevFunc)
{
    expr->liveness(prevFunc);
}
void Array::liveness(std::string prevFunc)
{
    for (auto *e : expr_list)
        e->liveness(prevFunc);
}

void Function::addExternal(TypeGraph *t)
{
    external.push_back(t);
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

    if (else_body != nullptr)
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
void ConstructorCall::liveness(std::string prevFunc)
{
    for (auto *e: expr_list)
    {
        e->liveness(prevFunc);
    }
}
void FunctionCall::liveness(std::string prevFunc)
{
    // Recursive call on parameters

    // If called function's definition has been analysed then
    // add its dependencies to prevFunc

    // If not then prevFunc is dependent on called function
}

void Match::liveness(std::string prevFunc)
{
    for (auto *c : clause_list)
    {
        c->liveness(prevFunc);
    }
}
void Clause::liveness(std::string prevFunc)
{
    // Open scope

    pattern->liveness(prevFunc);
    expr->liveness(prevFunc);

    // Close scope
}
void PatternId::liveness(std::string prevFunc)
{
    // Insert to table
}