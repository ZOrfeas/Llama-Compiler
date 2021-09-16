#include "options.hpp"

int LongOption::count = 130; // so that it doesn't hit any ascii codes
OptionList optionList;

Option::Option(int val, std::string name, std::string description, int has_arg)
    : val(val), name(name), description(description), has_arg(has_arg) {}
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
std::string Option::getOptarg()
{
    return optarg;
}
void Option::setOptarg(std::string s)
{
    optarg = s;
}
bool Option::isActivated()
{
    return activated;
}
void Option::activate()
{
    activated = true;
}
void Option::deactivate()
{
    activated = false;
}

ShortOption::ShortOption(char c, std::string description)
    : Option(c, std::string(1, c), description) { optionList.addShortOption(this); }

LongOption::LongOption(std::string name, std::string description, int has_arg)
    : Option(count++, name, description, has_arg) { optionList.addLongOption(this); }
struct option LongOption::getStructOption()
{
    return {name.c_str(), has_arg, NULL, val};
}

// Initialisations

LongOption 
    // Main options
    optimise("O", "Produces code and runs optimisations on it"),
    llvmIR("i", "Prints LLVM IR code"),
    printObjectCode("f", "Prints object code"),
    printAssemblyCode("S", "Prints assembly code"),
    outputFile("o", "Prints output to file specified", required_argument),

    // Auxiliary options for debug
    ast("ast", "Prints the whole AST produced by the syntactical analysis"),
    idTypes("idtypes", "Prints table with the names of variables and functions and their types"),
    inferenceLogs("inflogs", "Shows inference logs"),
    tableLogs("tlogs", "Shows symbol, type and constructor table logs"),
    printSuccess("success", "Prints success message as opposed to being silent when compilation succeeds"),
    
    // Options that allow the user to directly instruct the frontend
    frontend("frontend", "Controls the frontend and takes arguments syntax, sem, inf, compile", required_argument),
    
    // Help
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
struct option *OptionList::getLongOptionArray()
{
    struct option *arr = new struct option[longOptions.size()];
    for (int i = 0; i < (int)longOptions.size(); i++)
    {
        arr[i] = longOptions[i]->getStructOption();
    }

    return arr;
}
void OptionList::parseOptions(int argc, char **argv)
{
    int index = 0, c;
    struct option *long_options = getLongOptionArray();
    std::string short_options = "";
    while ((c = getopt_long_only(argc, argv, short_options.c_str(), long_options, &index)) != -1)
    {
        //std::cout << long_options[index].name << std::endl;

        // Check long options
        for (auto l : longOptions)
        {
            if (c == l->getVal())
            {
                l->activate();
                if (optarg) l->setOptarg(optarg);
                break;
            }
        }

        // Reset index
        index = 0;
    }

    // Print help message and exit immediately
    if (help.isActivated())
    {
        std::cout << "Usage ./llamac [options] < file" << std::endl;
        std::cout << std::endl;
        std::cout << "Options:\n";

        for (auto l : longOptions)
        {
            std::cout << "  " << std::left << std::setfill(' ') << std::setw(20) << "-" + l->getName()
                      << std::left << l->getDescription()
                      << std::endl;
        }

        std::cout << std::endl;
        std::cout << "Compiler might use files a.ll, a.o in which case they will be truncated" << std::endl;
        std::cout << std::endl;
        exit(0);
    }
}
void OptionList::executeOptions(Program *p)
{
    // If output file is provided freopen stdout to it
    std::string filename = "";
    if(outputFile.isActivated())
    {
        filename = outputFile.getOptarg();
        if(filename == "") 
        {
            std::cout << "No file provided" << std::endl;
            exit(1);
        }
        
        std::freopen(filename.c_str(), "w", stdout);
    }

    bool syntax = false, sem = false, inference = false, compile = false;
    if(frontend.isActivated()) 
    {
        std::string stage = frontend.getOptarg();

        if (stage == "syntax") 
            syntax = true;
        else if (stage == "sem")
            syntax = sem = true;
        else if (stage == "inf") 
            syntax = sem = inference = true;
        else if (stage == "compile")
            syntax = sem = inference = compile = true;
        else 
        {
            std::cout << "Argument " << "\"" + stage + "\"" << " passed to frontend is invalid" << std::endl;
            exit(1); 
        }
    }
    else 
    {
        syntax = sem = inference = compile = true;
    }

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

    if (syntax)
    {
        if (ast.isActivated())
        {
            //printHeader("AST");
            std::cout << *p << std::endl;
            //std::cout << std::endl;
        }
    }
    if (sem)
    {
        p->sem();
    }
    if (inference)
    {
        if (idTypes.isActivated())
        {
            //printHeader("Types of identifiers");
            p->printIdTypeGraphs();
            //std::cout << std::endl;
        }
        
        // this is required to print idTypeGraphs even if inference fails
        bool infSuccess = inf.solveAll(false);
        if (!infSuccess) exit(1);
    }
    if (compile)
    {
        bool opt = optimise.isActivated();
        p->start_compilation("a.ll", opt);
        
        if (llvmIR.isActivated())
        {
            p->printLLVMIR();
        }
        if (printAssemblyCode.isActivated())
        {
            p->emitAssemblyCode();
        }
        if (printObjectCode.isActivated())
        {
            if(filename == "")
                p->emitObjectCode("a.o");
            else
                p->emitObjectCode(filename.c_str());
        }
    }
}
