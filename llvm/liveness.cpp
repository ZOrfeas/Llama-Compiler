#include "ast.hpp"

/*
 * Liveness analysis for functions in order to determine 
 * what symbols they access from previous scopes.
 * 
 * Every node is passed the name of the function definition
 * inside which it exists so that it can add external dependencies
 * to the function.
 */

int LivenessEntry::getScope()
{
    return scope;
}
void LivenessEntry::visit()
{
    visited = true;
}
bool LivenessEntry::isVisited()
{
    return visited;
}
LivenessEntry::LivenessEntry(int scope)
    : scope(scope) {}
LivenessEntryDef::LivenessEntryDef(int scope, Def *symbolDef)
    : LivenessEntry(scope), symbolDef(symbolDef) {}
LivenessEntryPar::LivenessEntryPar(int scope, Par *symbolPar)
    : LivenessEntry(scope), symbolPar(symbolPar) {}
LivenessEntryFor::LivenessEntryFor(int scope, For *symbolFor)
    : LivenessEntry(scope), symbolFor(symbolFor) {}
LivenessEntryPatternId::LivenessEntryPatternId(int scope, PatternId *symbolPatternId)
    : LivenessEntry(scope), symbolPatternId(symbolPatternId) {}

Def* LivenessEntryDef::getNode()
{
    return symbolDef;
}
Par* LivenessEntryPar::getNode()
{
    return symbolPar;
}
For* LivenessEntryFor::getNode()
{
    return symbolFor;
}
PatternId* LivenessEntryPatternId::getNode()
{
    return symbolPatternId;
}

std::string LivenessEntryDef::getId()
{
    return symbolDef->getId();
}
std::string LivenessEntryPar::getId()
{
    return symbolPar->getId();
}
std::string LivenessEntryFor::getId()
{
    return symbolFor->getId();
}
std::string LivenessEntryPatternId::getId()
{
    return symbolPatternId->getId();
}

TypeGraph* LivenessEntryDef::getTypeGraph()
{
    return symbolDef->getTypeGraph();
}
TypeGraph* LivenessEntryPar::getTypeGraph()
{
    return symbolPar->get_TypeGraph();
}
TypeGraph* LivenessEntryFor::getTypeGraph() 
{
    return type_int;
}
TypeGraph* LivenessEntryPatternId::getTypeGraph()
{
    return symbolPatternId->getTypeGraph();
}

/*
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
*/

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

int getCurrScope()
{
    return LTable.getCurrScope();
}
int getScopeOf(std::string name)
{
    LivenessEntry *l = LTable[name];
    return l->getScope();
}
int isInScopeOf(std::string id, Function *func)
{
    int idScope = getScopeOf(id);
    int funcScope = (func == nullptr) ? 0 : func->getScope();

    return (funcScope < idScope);
}
void openScopeOnLTable()
{
    LTable.openScope();
}
void closeScopeOnLTable()
{
    LTable.closeScope();
}

void insertDefToLTable(Def *d)
{
    int scope = LTable.getCurrScope();
    LTable.insert({d->getId(), new LivenessEntryDef(scope, d)});
}
void insertParToLTable(Par *p)
{
    int scope = LTable.getCurrScope();
    LTable.insert({p->getId(), new LivenessEntryPar(scope, p)});
}
void insertForToLTable(For *f)
{
    int scope = LTable.getCurrScope();
    LTable.insert({f->getId(), new LivenessEntryFor(scope, f)});
}
void insertPatternIdToLTable(PatternId *p)
{
    int scope = LTable.getCurrScope();
    LTable.insert({p->getId(), new LivenessEntryPatternId(scope, p)});
}

LivenessEntry *lookupSymbolOnLTable(std::string name)
{
    return LTable[name];
}
void makeVisitedOnLTable(std::string func)
{
    LTable[func]->visit();
}

// Check whether this name belongs to prevFunc's scope
void checkScopeAndAddExternal(std::string name, Function *f)
{
    if(!isInScopeOf(name, f))
    {
        f->addExternal(lookupSymbolOnLTable(name));        
    }
}

/*******************************************************/
const std::string progFunc = "PROGRAM";

// By default do nothing
void AST::liveness(Function *prevFunc)
{
    return;
}

void Program::liveness(Function *prevFunc)
{
    // Recursive call using nullptr as function
    for (auto *d : definition_list)
    {
        d->liveness(nullptr);
    }
}
void Letdef::liveness(Function *prevFunc)
{
    if (recursive)
    {
        // Insert functions to table
        for(auto *d: def_list)
        {
            if(d->isDef())
            {
                insertDefToLTable(dynamic_cast<Def *>(d));
            } 
        }

        // Call liveness on each def and make them visited
        for(auto *d: def_list)
        {
            d->liveness(prevFunc);
            makeVisitedOnLTable(d->getId());
        }

        /*
        //! Inefficient but works
        // Move all external to the first function
        Function *firstFunc = dynamic_cast<Function *>(def_list[0]);
        for (auto *d: def_list)
        {
            Function *f = dynamic_cast<Function *>(d);
            insertExternalToFrom(firstFunc, f);
        }
        
        // From the first function give them everywhere
        for (auto *d: def_list)
        {
            Function *f = dynamic_cast<Function *>(d);
            insertExternalToFrom(f, firstFunc);
        }
        */
    }
    else
    {
        // Call liveness on each def
        for(auto *d: def_list)
        {
            d->liveness(prevFunc);
        }

        // Insert symbols to table and make them visited
        for(auto *d: def_list)
        {
            if(d->isDef())
            {
                insertDefToLTable(dynamic_cast<Def *>(d));
            } 

            makeVisitedOnLTable(d->getId());        
        }
    }
}

TypeGraph *DefStmt::getTypeGraph() 
{
    return nullptr;
}
TypeGraph *Def::getTypeGraph()
{
    return TG;
}
void Constant::liveness(Function *prevFunc)
{
    expr->liveness(prevFunc);
}
void Array::liveness(Function *prevFunc)
{
    for (auto *e : expr_list)
    {
        e->liveness(prevFunc);
    }
}

void Function::addExternal(LivenessEntry *l)
{
    // Don't add a function to its own external
    if(this == l->getNode()) 
    {
        return;
    }

    std::string id = l->getId();

    // Only add it if it hasn't already been added
    if(external.find(id) == external.end()) 
    {
        external[id] = l;
    }
}
void insertExternalToFrom(Function *funcDependent, Function *func)
{
    int funcDependentScope = funcDependent->getScope();
    for(auto const it: func->external)
    {
        // If symbol has been defined inside dependent func skip it
        int extSymbolScope = it.second->getScope();
        if(funcDependentScope < extSymbolScope)
        {
            continue;
        }
        
        // Otherwise add it to dependent func too
        funcDependent->external[it.first] = it.second;
    }
    
    //funcDependent->external.insert(func->external.begin(), func->external.end());
}
std::map<std::string, LivenessEntry *> Function::getExternal()
{
    return external;
}
void Function::setScope(int s)
{
    scope = s;
}
int Function::getScope()
{
    return scope;
}
void Function::liveness(Function *prevFunc)
{
    // Save scope of function
    scope = getCurrScope();

    // Create new scope
    openScopeOnLTable();

    // Insert parameters to table
    for(auto p: par_list)
    {
        insertParToLTable(p);
    }
    
    // Recursive call to the body passing this function
    expr->liveness(this);
    
    if(prevFunc)
    {
        insertExternalToFrom(prevFunc, this);
    }

    // Close scope
    closeScopeOnLTable();
}

void LetIn::liveness(Function *prevFunc)
{
    letdef->liveness(prevFunc);
    expr->liveness(prevFunc);
}
void BinOp::liveness(Function *prevFunc)
{
    lhs->liveness(prevFunc);
    rhs->liveness(prevFunc);
}
void UnOp::liveness(Function *prevFunc)
{
    expr->liveness(prevFunc);
}

void While::liveness(Function *prevFunc)
{
    cond->liveness(prevFunc);
    body->liveness(prevFunc);
}
void For::liveness(Function *prevFunc)
{
    // Open scope
    openScopeOnLTable();

    // Insert node to table
    insertForToLTable(this);

    // Recursive calls
    start->liveness(prevFunc);
    finish->liveness(prevFunc);
    body->liveness(prevFunc);

    // Close scope
    closeScopeOnLTable();
}
void If::liveness(Function *prevFunc)
{
    cond->liveness(prevFunc);
    body->liveness(prevFunc);

    if (else_body != nullptr)
        else_body->liveness(prevFunc);
}

void Dim::liveness(Function *prevFunc)
{
    if(!prevFunc) return;

    // Check whether this array belongs to prevFunc's scope
    checkScopeAndAddExternal(id, prevFunc);
}
void ArrayAccess::liveness(Function *prevFunc)
{
    // Recursive call on indices
    for(auto *e: expr_list)
    {
        e->liveness(prevFunc);
    }

    if(!prevFunc) return;

    // Check whether this array belongs to prevFunc's scope
    checkScopeAndAddExternal(id, prevFunc);
}
void ConstantCall::liveness(Function *prevFunc)
{
    if(!prevFunc) return;

    // Check whether this constant belongs to prevFunc's scope
    checkScopeAndAddExternal(id, prevFunc);
}
void ConstructorCall::liveness(Function *prevFunc)
{
    for (auto *e: expr_list)
    {
        e->liveness(prevFunc);
    }
}
void FunctionCall::liveness(Function *prevFunc)
{
    // Recursive call on arguments
    for(auto *e: expr_list)
    {
        e->liveness(prevFunc);
    }

    if(!prevFunc) 
    {
        return;
    }
    
    // Check whether this function belongs to prevFunc's scope
    checkScopeAndAddExternal(id, prevFunc);
}

void Match::liveness(Function *prevFunc)
{
    for (auto *c : clause_list)
    {
        c->liveness(prevFunc);
    }
}
void Clause::liveness(Function *prevFunc)
{
    // Open scope
    openScopeOnLTable();

    pattern->liveness(prevFunc);
    expr->liveness(prevFunc);

    // Close scope
    closeScopeOnLTable();
}
void PatternId::liveness(Function *prevFunc)
{
    // Insert to table
    insertPatternIdToLTable(this);
}