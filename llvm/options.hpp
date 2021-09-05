#include <ctype.h>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <getopt.h>
#include <iomanip>
#include <string>
#include <vector>

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
    ShortOption(char c, std::string description)
    : Option(c, std::to_string(c), description) {}
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