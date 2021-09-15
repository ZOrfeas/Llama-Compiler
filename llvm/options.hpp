#pragma once

#include <iostream>
#include <ctype.h>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <getopt.h>
#include <iomanip>
#include <string>
#include <vector>

#include "ast.hpp"
#include "infer.hpp"
#include "symbol.hpp"

class Option
{
protected:
    bool activated = false;
    int val;
    std::string name, description;

    int has_arg;
    // Will be filled if an argument is given
    std::string optarg = "";

public:
    Option(int val, std::string name, std::string description, int has_arg = no_argument);
    int getVal();
    std::string getName();
    std::string getDescription();
    std::string getOptarg();
    void setOptarg(std::string s);
    bool isActivated();
    void activate();
    void deactivate();
};

class ShortOption
    : public Option
{
public:
    ShortOption(char c, std::string description);
};

class LongOption
    : public Option
{
protected:
    static int count;
public:
    LongOption(std::string name, std::string description, int has_arg = no_argument);
    struct option getStructOption();
};

class OptionList
{
protected:
    std::vector<ShortOption *> shortOptions;
    std::vector<LongOption *> longOptions;
public:
    OptionList();
    void addShortOption(ShortOption *s);
    void addLongOption(LongOption *l);
    struct option* getLongOptionArray();
    void parseOptions(int argc, char **argv);
    void executeOptions(Program *p);
};

extern OptionList optionList;
extern LongOption printSuccess;