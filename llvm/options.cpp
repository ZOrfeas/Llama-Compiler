#include "options.hpp"

int LongOption::count = 0;

Option::Option(int val, std::string name, std::string description)
    : val(val), name(name), description(description) {}
int Option::getVal()
{
    return val;
}
std::string Option::getName()
{
    return name;
}
std::string Option::getDescription()
{
    return description;
}
bool Option::isActivated()
{
    return activated;
}
void Option::activate()
{
    activated = true;
}

ShortOption::ShortOption(char c, std::string description)
    : Option(c, std::to_string(c), description) {}

LongOption::LongOption(std::string name, std::string description)
    : Option(val, name, description) { val++; }
struct option LongOption::getStructOption()
{
    return {name.c_str(), no_argument, NULL, val};
}

// Initialisations

ShortOption *syntax     = new ShortOption('s', "Checks only if the program is syntactically correct"),
            *sem        = new ShortOption('a', "Checks if the program is also semantically correct"),
            *inference  = new ShortOption('i', "Performs inference to resolve unknown types"),
            *compile    = new ShortOption('c', "Produces code");

LongOption *tableLogs       = new LongOption("tableLogs", "Shows symbol, type and constructor table logs"),
           *inferenceLogs   = new LongOption("inferenceLogs", "Shows inference logs"),
           *printTable      = new LongOption("printTable", "Prints table with the names of variables and functions and their types"),
           *printAST        = new LongOption("printAST", "Prints the whole AST produced by the syntactical analysis"),
           *printIRCode     = new LongOption("printIRCode", "Prints LLVM IR code"),
           *printObjectCode = new LongOption("printObjectCode", "Prints object code"),
           *printFinalCode  = new LongOption("printFinalCode", "Prints final code"),
           *printSuccess    = new LongOption("printSuccess", "Prints success message as opposed to being silent when compilation succeeds"),
           *help            = new LongOption("help", "Shows all options and their funcionality");

std::vector<Option *> optionList = 
{ 
    syntax,   
    sem,      
    inference,
    compile,  

    tableLogs,      
    inferenceLogs,
    printTable,     
    printAST,       
    printIRCode,    
    printObjectCode,
    printFinalCode, 
    printSuccess,   
    help
};

int option_index = 0;
struct option long_options[] = 
{
    tableLogs->getStructOption(),
    inferenceLogs->getStructOption(),
    printTable->getStructOption(),
    printAST->getStructOption(),
    printIRCode->getStructOption(),
    printObjectCode->getStructOption(),
    printFinalCode->getStructOption(),
    printSuccess->getStructOption(),
    help->getStructOption()
};

