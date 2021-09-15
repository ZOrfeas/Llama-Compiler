#include <iostream>
#include <algorithm>
#include "infer.hpp"

using std::vector;
using std::pair;
using std::map;
using std::string;
using std::to_string;
/********************/
/**** Constraint ****/
/********************/

Constraint::Constraint(TypeGraph *lhs, TypeGraph *rhs, int lineno, string msg)
: lhs(lhs), rhs(rhs), lineno(lineno), msg(msg) {}
TypeGraph* Constraint::getLhs() { return lhs; }
TypeGraph* Constraint::getRhs() { return rhs; }
std::string Constraint::getMsg() { return msg; }
int Constraint::getLineNo() { return lineno; }
string Constraint::stringify() {
    return "(line " + to_string(lineno) + ") " + 
            inf.tryApplySubstitutions(lhs)->stringifyType() + 
            " == " + inf.tryApplySubstitutions(rhs)->stringifyType() +
            " (orig. " + lhs->stringifyType() + " == " + rhs->stringifyType() + ")";
}
Constraint::~Constraint() {}

/*******************/
/****  Inferer  ****/
/*******************/

Inferer::Inferer(bool debug): constraints(new vector<Constraint *>()),
substitutions(new map<string, TypeGraph *>()), debug(debug) {}
Inferer::~Inferer() {
    //TODO: Figure out what needs to be deleted
}
void Inferer::log(string msg) {
    std::cout << "\033[1m\033[32mInferer\033[0m: ";
    std::cout << msg << std::endl;
}
void Inferer::error(string msg) {
    log("Error: " + msg);
    exit(1);
}
vector<Constraint *>* Inferer::getConstraints() { return constraints; }
map<string, TypeGraph *>* Inferer::getSubstitutions() {
    return substitutions;
}

void Inferer::addConstraint(TypeGraph *lhs, TypeGraph *rhs, int lineno, string msg) {
    if (debug)
        log("Adding constraint, " + lhs->stringifyType()
            + " == " + rhs->stringifyType());
    lhs = tryApplySubstitutions(lhs);
    rhs = tryApplySubstitutions(rhs);
    auto newConstraint = new Constraint(lhs, rhs, lineno, msg);
    constraints->push_back(newConstraint);
}
bool Inferer::isValidSubstitution(TypeGraph *unknownType, TypeGraph* candidateType) {
    bool invalid = (
        (!unknownType->canBeArray() && candidateType->isArray()) ||
        (!unknownType->canBeFunc() && candidateType->isFunction()) ||
        (unknownType->onlyIntCharFloat() && !candidateType->isUnknown() && 
            !candidateType->isInt() && !candidateType->isChar() && !candidateType->isFloat())
    );
    return !invalid;
}
bool Inferer::isOrOccurs(TypeGraph* unknownType, TypeGraph* candidateType) {
    return unknownType->equals(candidateType)
           || occurs(unknownType, candidateType);
}
bool Inferer::occurs(TypeGraph *unknownType, TypeGraph* candidateType) {
    if (candidateType->isArray() || candidateType->isRef()) {
        return isOrOccurs(unknownType, 
                  tryApplySubstitutions(candidateType->getContainedType()));
    } else if (candidateType->isFunction()) { // if candidateType is a function type
        for (int i = 0; i < candidateType->getParamCount(); i++) {
            if (isOrOccurs(unknownType, 
                  tryApplySubstitutions(candidateType->getParamType(i)))) // check if isOrOccurs with every parameter
                return true; 
        }
        return isOrOccurs(unknownType,
                 tryApplySubstitutions(candidateType->getResultType())); // check if isOrOccurs with the result type
    } else {
        return false; // in any other case unknownType cannot occur in candidateType (equality has been covered)
    }
}
void Inferer::saveSubstitution(string unknownTmpName, TypeGraph *resolvedTypeGraph) {
    auto itr = substitutions->find(unknownTmpName);
    if (itr == substitutions->end()) 
        error("Internal, free type name " +
              unknownTmpName + " not found in substitutions map");
    if (itr->second != nullptr)
        error("Internal, free type name " +
              unknownTmpName + " should not be substituted twice");
    itr->second = resolvedTypeGraph;
}
void Inferer::trySubstitute(TypeGraph *unknownType, TypeGraph *candidateType, int lineno) {
    if (debug)
        log("Replacing " + unknownType->stringifyType() +
            " at line " + to_string(lineno) +
            " with " + candidateType->stringifyType());
    if (!isValidSubstitution(unknownType, candidateType)) // Constraint violated, exit
        error("Substitution at line " + to_string(lineno) +
              " violated constraint");
    if (occurs(unknownType, candidateType)) // Recursive unknown type found, exit
        error("Constraint at line " + to_string(lineno) +
              " implied recursive unknown type (occurs check)");
    if (candidateType->isUnknown())
        candidateType->copyConstraintFlags(unknownType);
    saveSubstitution(unknownType->getTmpName(), candidateType);
}
// utility function to not bloat the main solveOne
bool areCompatibleArraysOrRefs(TypeGraph *a, TypeGraph *b) {
    
    if (a->isRef() && b->isRef()) {
        return true;
    } else if (a->isArray() && b->isArray()) {
        if (a->getDimensions() != -1 && b->getDimensions() != -1 &&
            (a->getDimensions() == b->getDimensions())) {
            return true; // this is the case were both the arrays are of known dims
        } else if (a->getDimensions() == -1 && b->getDimensions() == -1) {
            // case were both arrays are of unknown dims
            if (a->getBound() >= b->getBound()) {
                b->changeBoundPtr(a->getBoundPtr()); // keep greater of two
            } else {
                a->changeBoundPtr(b->getBoundPtr());
            }
            return true; // they always match
        } else if (a->getDimensions() == -1) { // only a is unknown
            if (b->getDimensions() < a->getBound()) { // incompatible known with unknown
                return false;
            } else { // compatible, make them fixed
                a->setDimensions(b->getDimensions());
                return true;
            }
        } else if (b->getDimensions() == -1) {
            if (a->getDimensions() < b->getBound()) {
                return false;
            } else {
                b->setDimensions(a->getDimensions());
                return true;
            }
        } // else falls through
    }
    return false;
}
TypeGraph* Inferer::isUnknown_Exists_HasSubstitution(TypeGraph *unknonwnType) {
    if (!unknonwnType->isUnknown())
        return nullptr;
    auto itr = substitutions->find(unknonwnType->getTmpName());
    if (itr == substitutions->end())
        error("Internal, free type name not found in substitutions map");
    return itr->second;
}
//TODO: Fiercely test
TypeGraph* Inferer::tryApplySubstitutions(TypeGraph* type) {
    vector<string> toChange;
    TypeGraph *next, *current = type;
    while ((next = isUnknown_Exists_HasSubstitution(current))){
        toChange.push_back(current->getTmpName());
        current = next;
    }
    for (auto &name: toChange) { // apply final substitution to all previous for performance
        (*substitutions)[name] = current;
    }
    return current;
}
TypeGraph* Inferer::deepSubstitute(TypeGraph* type) {
    TypeGraph *temp = tryApplySubstitutions(type);
    if (!temp->isFunction() && !temp->isArray() && !temp->isRef()) {
        return temp;
    } else if (temp->isArray() || temp->isRef()) {
        temp->changeInner(deepSubstitute(temp->getContainedType()));
        return temp;
    } else { // if isFunction()
        for (int i = 0; i < temp->getParamCount(); i++) {
            temp->changeInner(deepSubstitute(temp->getParamType(i)), i);
        }
        temp->changeInner(deepSubstitute(temp->getResultType()), temp->getParamCount());
        return temp;
    }
}
TypeGraph* Inferer::getSubstitutedLhs(Constraint *constraint) {
    return tryApplySubstitutions(constraint->getLhs());
}
TypeGraph* Inferer::getSubstitutedRhs(Constraint *constraint) {
    return tryApplySubstitutions(constraint->getRhs());
}
TypeGraph* Inferer::getSubstitutedType(TypeGraph* unknownType) {
    return tryApplySubstitutions(unknownType);
}
void Inferer::solveOne(Constraint *constraint) {
    TypeGraph *lhsTypeGraph = getSubstitutedLhs(constraint),
              *rhsTypeGraph = getSubstitutedRhs(constraint);
    if (debug)
        log("Processing constraint: " + constraint->stringify());
    if (lhsTypeGraph->equals(rhsTypeGraph)) {
        if(debug) log("Constraint equates already equal types, ignoring...");    
    } else if (lhsTypeGraph->isUnknown()) {                                        // lhs is unknown
        trySubstitute(lhsTypeGraph, rhsTypeGraph, constraint->getLineNo());
    } else if (rhsTypeGraph->isUnknown()) {                                         // rhs is unknown
        trySubstitute(rhsTypeGraph, lhsTypeGraph, constraint->getLineNo());
    } else if (lhsTypeGraph->isFunction() && rhsTypeGraph->isFunction()) {          // both are function types
        if (lhsTypeGraph->getParamCount() != rhsTypeGraph->getParamCount()) {           // they have unequal parameter counts
            error("Line " + to_string(constraint->getLineNo()) +
                  " non-equal parameter counts");     
         } else {                                                                       // the have equal parameter counts
            if (debug)
                log("Constraining arguments of " + lhsTypeGraph->stringifyType() +
                    " and " + rhsTypeGraph->stringifyType() +
                    " at line " + to_string(constraint->getLineNo()));
            for (int i = 0; i < lhsTypeGraph->getParamCount(); i++) {
                addConstraint(lhsTypeGraph->getParamType(i), rhsTypeGraph->getParamType(i),
                              constraint->getLineNo(), constraint->getMsg()); // insert constraints for parameter types
            }
            addConstraint(lhsTypeGraph->getResultType(), rhsTypeGraph->getResultType(),
                          constraint->getLineNo(), constraint->getMsg()); // insert constraint for result types
        }
    } else if (areCompatibleArraysOrRefs(lhsTypeGraph, rhsTypeGraph)) {             // are both refs, or arrays of same dimensions
        addConstraint(lhsTypeGraph->getContainedType(), rhsTypeGraph->getContainedType(),
                      constraint->getLineNo(), constraint->getMsg());
    } else {                                                                        // any other case type check/inference fails
        if (debug)
            log("Failed on constraint " + constraint->stringify());
        std::cout << "Error at line " + to_string(constraint->getLineNo()) + ": "
                  << constraint->getMsg() << std::endl;
        exit(1);
    }
}
void Inferer::initSubstitution(string name) {
    substitutions->insert({name, nullptr});
}
bool Inferer::checkAllSubstituted(bool err) {
    bool success = true;
    if(debug) log("Validating all unknown types where successfuly infered...");
    for (auto &pair: (*substitutions)) {
        if (!pair.second || tryApplySubstitutions(pair.second)->isUnknown()) {
            log("Type " + pair.first + " could not be infered");
            if (err) exit(1);
            else success = false;
        } 
    }
    if (success)
        if(debug) log("Inference successful");
    return success;
}
bool Inferer::solveAll(bool err) {
    std::reverse(constraints->begin(), constraints->end());
    Constraint *holder;
    while(!constraints->empty()) {
        holder = constraints->back();
        constraints->pop_back();
        solveOne(holder);
    }
    return checkAllSubstituted(err);
}

void Inferer::enable_logs() { debug = true; }

bool    inferer_logs = false;
Inferer inf(inferer_logs);