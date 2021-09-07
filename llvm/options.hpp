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

#include "lexer.hpp"
#include "ast.hpp"
#include "infer.hpp"
#include "symbol.hpp"

class Option
{
protected:
    bool activated = false;
    int val;
    std::string name, description;
public:
    Option(int val, std::string name, std::string description);
    int getVal();
    std::string getName();
    std::string getDescription();
    bool isActivated();
    void activate();
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
    static int count; // shouldn't go higher than 40
public:
    LongOption(std::string name, std::string description);
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