#ifndef __TYPES_HPP__
#define __TYPES_HPP__

#include <vector>
#include <string>

enum class graphType { TYPE_unknown, TYPE_unit, TYPE_int, TYPE_float, TYPE_bool,
            TYPE_char, TYPE_ref, TYPE_array, TYPE_function, TYPE_custom, TYPE_record };

// Forward declarations
class CustomTypeGraph;
class ConstructorTypeGraph;

/** Base Type Graph class from which all others are derived */
class TypeGraph {
    graphType t;
    void wrongCall(std::string functionName);
public:
    TypeGraph(graphType t);
    graphType const & getSubClass();
    virtual std::string stringifyType();
    void log(std::string msg);
    bool isFunction();
    bool isArray();
    bool isRef();
    bool isInt();
    bool isUnit();
    bool isBool();
    bool isChar();
    bool isFloat();
    bool isCustom();
    bool isConstructor();
    bool isUnknown();
    bool isBasic();
    bool isDeletable();
    virtual bool equals(TypeGraph *o) = 0;
    virtual TypeGraph* getContainedType();
    virtual int getDimensions();
    virtual void setDynamic();
    virtual void setAllocated();
    virtual void resetDynamic();
    virtual void resetAllocated();
    virtual bool isAllocated();
    virtual bool isDynamic();
    virtual std::vector<TypeGraph *>* getParamTypes();
    virtual TypeGraph* getResultType();
    virtual int getParamCount();
    virtual void addParam(TypeGraph *param, bool push_back = true);
    virtual TypeGraph* getParamType(unsigned int index);
    virtual std::vector<TypeGraph *>* getFields();
    virtual void addField(TypeGraph *field);
    virtual void setTypeGraph(CustomTypeGraph *owningType);
    virtual CustomTypeGraph* getCustomType();
    virtual int getFieldCount();
    virtual TypeGraph* getFieldType(unsigned int index);
    virtual int getConstructorCount();
    virtual void addConstructor(ConstructorTypeGraph *constructor);
    virtual std::vector<ConstructorTypeGraph *>* getConstructors();
    virtual std::string getTmpName();
    virtual unsigned long getId();
    virtual bool canBeArray();
    virtual bool canBeFunc();
    virtual bool onlyIntCharFloat();
    virtual void setIntCharFloat();
    virtual ~TypeGraph() {}
};
/************************************************************/

class UnknownTypeGraph : public TypeGraph {
    unsigned long tmp_id;
    static unsigned long curr; // holds next-up tmp_name
    bool can_be_array, can_be_func, only_int_char_float;
public:
    //TODO: not complete
    UnknownTypeGraph(bool can_be_array = false, bool can_be_func = false,
        bool only_int_char_float = false);
    std::string stringifyType() override;
    unsigned long getId() override;
    std::string getTmpName() override;
    bool canBeArray() override;
    bool canBeFunc() override;
    bool onlyIntCharFloat() override;
    void setIntCharFloat() override;
    bool equals(TypeGraph *o) override; 
    ~UnknownTypeGraph() {}
};
/************************************************************/
/** Base Basic Type Graph class from which all basic type are derived */
class BasicTypeGraph : public TypeGraph {
public:
    BasicTypeGraph(graphType t);
    bool equals(TypeGraph *o);
    virtual ~BasicTypeGraph() {}
};
class UnitTypeGraph : public BasicTypeGraph {
public:
    UnitTypeGraph();
    std::string stringifyType() override;
    ~UnitTypeGraph() {}
};
class IntTypeGraph : public BasicTypeGraph {
public:
    IntTypeGraph();
    std::string stringifyType() override;
    ~IntTypeGraph() {}
};
class CharTypeGraph : public BasicTypeGraph {
public:
    CharTypeGraph();
    std::string stringifyType() override;
    ~CharTypeGraph() {}
};
class BoolTypeGraph : public BasicTypeGraph {
public:
    BoolTypeGraph();
    std::string stringifyType() override;
    ~BoolTypeGraph() {}
};
class FloatTypeGraph : public BasicTypeGraph {
public:
    FloatTypeGraph();
    std::string stringifyType() override;
    ~FloatTypeGraph() {}
};
/** Complex Type Graphs */
/************************************************************/
class ArrayTypeGraph : public TypeGraph {
    TypeGraph *Type;
    int dimensions;
    std::string stringifyDimensions();
public:
    ArrayTypeGraph(int dimensions, TypeGraph *containedType);
    std::string stringifyType() override;
    TypeGraph* getContainedType();
    bool equals(TypeGraph *o);
    int getDimensions() override;
    ~ArrayTypeGraph();
};
class RefTypeGraph : public TypeGraph {
    TypeGraph *Type;
    bool allocated, dynamic;
public:
    RefTypeGraph(TypeGraph *refType,
        bool allocated = false, bool dynamic = false);
    std::string stringifyType() override;
    TypeGraph* getContainedType() override;
    void setAllocated() override;
    void setDynamic() override;
    void resetAllocated() override;
    void resetDynamic() override;
    bool isAllocated() override;
    bool isDynamic() override;
    bool equals(TypeGraph *o) override;
    ~RefTypeGraph();
};
class FunctionTypeGraph : public TypeGraph {
    std::vector<TypeGraph *> *paramTypes; 
    TypeGraph *resultType;
    std::string stringifyParams();
public:
    FunctionTypeGraph(TypeGraph *resultType);
    std::string stringifyType() override;
    std::vector<TypeGraph *>* getParamTypes() override;
    TypeGraph* getResultType() override;
    int getParamCount() override;
    /** Utility method for creating more complex FunctionTypeGraphs 
     * @param push_back If true then param is appended, else inserted at start */
    void addParam(TypeGraph *param, bool push_back = true) override;
    TypeGraph* getParamType(unsigned int index) override;
    bool equals(TypeGraph *o) override;
    ~FunctionTypeGraph();
};

/** This represents a singular constructor */
class ConstructorTypeGraph : public TypeGraph {
    std::vector<TypeGraph *> *fields;
    CustomTypeGraph *customType;
public:
    ConstructorTypeGraph();
    std::string stringifyType() override;
    std::vector<TypeGraph *>* getFields() override;
    void addField(TypeGraph *field) override;
    void setTypeGraph(CustomTypeGraph *owningType) override;
    CustomTypeGraph* getCustomType() override;
    int getFieldCount() override;
    TypeGraph* getFieldType(unsigned int index) override;
    bool equals(TypeGraph *o) override;
    ~ConstructorTypeGraph();
};
class CustomTypeGraph : public TypeGraph {
    std::string name;
    std::vector<ConstructorTypeGraph *> *constructors;
public:
    CustomTypeGraph(std::string name, 
                    std::vector<ConstructorTypeGraph *> *constructors = new std::vector<ConstructorTypeGraph *>());
    std::string stringifyType() override;
    std::vector<ConstructorTypeGraph *>* getConstructors() override;
    int getConstructorCount() override;
    void addConstructor(ConstructorTypeGraph *constructor) override;
    //! possibly too strict. keep an eye out
    bool equals(TypeGraph *o) override;
    ~CustomTypeGraph();
};

#endif