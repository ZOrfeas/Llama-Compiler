#ifndef __INFER_HPP__
#define __INFER_HPP__

#include <string>
#include <vector>
#include <map>
#include "types.hpp"

/** Represents a single constraint */
class Constraint {
    TypeGraph *lhs, *rhs;
    int lineno;
    std::string msg;
public:
    Constraint(TypeGraph *lhs, TypeGraph *rhs, int lineno, std::string msg);
    /** Returns a container compatible reference wrapper to the lhs of the constraint */
    TypeGraph* getLhs();
    /** Returns a container compatible reference wrapper to the rhs of the constraint */
    TypeGraph* getRhs();
    std::string getMsg();
    int getLineNo();
    std::string stringify();
    ~Constraint();
};

class Inferer {
    // holds all the constraints of the program
    // Changes/Empties while inference runs
    std::vector<Constraint *> *constraints;
    // Maps free type variables with what they should become
    // Fills up while inference runs
    std::map<std::string, TypeGraph *> *substitutions;
    // // Finds free type names and fills vector with them
    // // always called when there should be no substitutions yet
    // void getFreeTypes(TypeGraph *fullType, std::vector<std::string> *names);
    // // Initializes empty substitutions for every free type name
    // void initSubstitutions(std::vector<std::string> *names);
    // checks if substitution would ignore any restraints
    bool isValidSubstitution(TypeGraph *unknownType, TypeGraph *candidateType);
    // looks up if it is unknown, the name is valid, and has been substituted,
    // returns null if no substitution and crashes if it doesn't exist
    TypeGraph* isUnknown_Exists_HasSubstitution(TypeGraph *unknownType);
    // saves a substitution of a currently non substituted free type name
    void saveSubstitution(std::string unknownTmpName, TypeGraph *resolvedTypeGraph);
    // checks if a substitution is valid and applies it, otherwise crashes
    void trySubstitute(TypeGraph *unknownType, TypeGraph *candidateType, int lineno);
    /** performs an "occurs check" @param unknownType type to look for @param candidateType type to look in */
    bool occurs(TypeGraph *unknownType, TypeGraph *candidateType);
    // helper for occurs check
    bool isOrOccurs(TypeGraph *unknownType, TypeGraph *candidateType);
    // Solves and removes a constraint if possible
    void solveOne(Constraint *constraint);
    // prints logs to stdout
    void log(std::string msg);
    // prints error to stdout and exits
    void error(std::string msg);
    bool debug;
public:
    Inferer(bool debug = false);
    std::vector<Constraint *>* getConstraints();
    std::map<std::string, TypeGraph *>* getSubstitutions();
    TypeGraph* getSubstitutedLhs(Constraint *constraint);
    TypeGraph* getSubstitutedRhs(Constraint *constraint);
    TypeGraph* getSubstitutedType(TypeGraph *unknownType);
    // applies as many substitutions as possible to the given type
    // and returns the "true" current typeGraph it has been resolve too, thus far
    TypeGraph* tryApplySubstitutions(TypeGraph* unknownType);
    TypeGraph* deepSubstitute(TypeGraph* unknownType);
    void solveAll(bool err = true);
    /** Stores a new constraint
     * @param lhs pointer to lhs
     * @param rhs pointer to rhs
     * @param lineno line where constraint was created
     */
    void addConstraint(TypeGraph *lhs, TypeGraph *rhs, int lineno, std::string msg = "");
    void initSubstitution(std::string name);
    void checkAllSubstituted(bool err = true);
    void enable_logs();
    ~Inferer();
};

extern Inferer inf;

#endif // __INFER_HPP__
