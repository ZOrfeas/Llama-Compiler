#include "options.hpp"

int LongOption::count = 130; // so that it doesn't hit any ascii codes
OptionList optionList;

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
    : Option(c, std::string(1, c), description) { optionList.addShortOption(this); }

LongOption::LongOption(std::string name, std::string description)
    : Option(count++, name, description) { optionList.addLongOption(this); }
struct option LongOption::getStructOption()
{
    return {name.c_str(), no_argument, NULL, val};
}

// Initialisations

// Short options must be created in correct order
ShortOption syntaxAnalysis('s', "Checks only if the program is syntactically correct"),
            semAnalysis('a', "Checks if the program is also semantically correct"),
            inferenceAnalysis('i', "Performs inference to resolve unknown types"),
            compile('c', "Produces code");

LongOption tableLogs("tableLogs", "Shows symbol, type and constructor table logs"),
           inferenceLogs("inferenceLogs", "Shows inference logs"),
           printTable("printTable", "Prints table with the names of variables and functions and their types"),
           printAST("printAST", "Prints the whole AST produced by the syntactical analysis"),
           printIRCode("printIRCode", "Prints LLVM IR code"),
           printObjectCode("printObjectCode", "Prints object code"),
           printFinalCode("printFinalCode", "Prints final code"),
           printSuccess("printSuccess", "Prints success message as opposed to being silent when compilation succeeds"),
           help("help", "Shows all options and their funcionality");


OptionList::OptionList()
{
    shortOptions = {};
    longOptions = {};
}
void OptionList::addShortOption(ShortOption *s)
{
    shortOptions.push_back(s);
}
void OptionList::addLongOption(LongOption *l)
{
    longOptions.push_back(l);
}
struct option* OptionList::getLongOptionArray()
{
    struct option *arr = new struct option[longOptions.size()];
    for(int i = 0; i < (int)longOptions.size(); i++)
    {
        arr[i] = longOptions[i]->getStructOption();
    }

    return arr;
}
void OptionList::parseOptions(int argc, char **argv)
{
    int option_index = 0;
    struct option *long_options = getLongOptionArray();

    std::string short_options = "";
    for(auto s: shortOptions) { short_options += s->getName(); }

    int c;
    int highestActivatedShortOption = -1;
    bool noShortOptions = true; // If there are no short options then run all stages
    while((c = getopt_long(argc, argv, short_options.c_str(), long_options, &option_index)) != -1)
    {   
        // Check long options
        for(auto l: longOptions)
        {
            if(c == l->getVal()) 
            {
                l->activate();
                break;
            }
        }

        // Check short options in reverse
        for(int i = shortOptions.size() - 1; i >= 0; i--)
        {
            auto s = shortOptions[i];

            if(c == s->getVal())
            {
                s->activate();
                noShortOptions = false;
                if(i > highestActivatedShortOption) 
                {
                    highestActivatedShortOption = i;
                }
            }
        }
    }

    // Print help message
    if(help.isActivated()) 
    {
        std::cout << "Usage ./llamac -[short options] --[long options] < file" << std::endl;
        std::cout << std::endl;
        std::cout << "Options:\n";

        for(auto s: shortOptions)
        {
            std::cout << "  " << std::left << std::setfill(' ') << std::setw(20) << "-" + s->getName()
                  << std::left << s->getDescription()
                  << std::endl;
        }
        for(auto l: longOptions)
        {
            std::cout << "  " << std::left << std::setfill(' ') << std::setw(20) << "--" + l->getName()
                  << std::left << l->getDescription()
                  << std::endl;
        }
        exit(0);
    }

    // If no short options are given then compile
    // Else activate until highest activated short option
    if(noShortOptions) 
    {
        for(auto s: shortOptions)
        {
            s->activate();
        }
    }
    else 
    {
        for(int i = 0; i <= highestActivatedShortOption; i++)
        {
            shortOptions[i]->activate();
        }
    }
}
void OptionList::executeOptions(Program *p)
{
    if (inferenceLogs.isActivated()) 
    {
        inf.enable_logs();
    }
    if (tableLogs.isActivated()) 
    {
        st.enable_logs();
        tt.enable_logs();
        ct.enable_logs();
    }
    if(syntaxAnalysis.isActivated())
    {
        if(printAST.isActivated()) 
        {   
            //printHeader("AST");
            std::cout << *p << std::endl; 
            std::cout << std::endl;
        }
    }
    if(semAnalysis.isActivated())
    {
        p->sem(); 
    }
    if(inferenceAnalysis.isActivated()) 
    {
        inf.solveAll(false);
        if(printTable.isActivated()) 
        {   
            //printHeader("Types of identifiers");
            p->printIdTypeGraphs();
            std::cout << std::endl;
        }
    } 

    if(compile.isActivated()) 
    {
        p->start_compilation("a.ll");
        if(printIRCode.isActivated()) p->printLLVMIR();
    }
}



