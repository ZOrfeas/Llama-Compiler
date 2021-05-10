.PHONY: default clean distclean

CXX=c++
CXXFLAGS=-Wall -std=c++11

default: compiler

lexer.cpp: lexer.l
	flex -s -o lexer.cpp lexer.l

parser.hpp parser.cpp: parser.y
	bison -dv --debug -Wall -o parser.cpp parser.y

# Add your dependencies here, e.g.
# parser.o: parser.cpp lexer.hpp symbol.hpp
lexer.o: lexer.cpp lexer.hpp parser.hpp
parser.o: parser.cpp lexer.hpp symbol.hpp
symbol.o: symbol.hpp

# compiler: lexer.o parser.o symbol.o
compiler: lexer.o parser.o symbol.o
	$(CXX) $(CXXFLAGS) -o llamac lexer.o parser.o

clean:
	$(RM) lexer.cpp parser.cpp parser.hpp parser.output *.o

distclean: clean
	$(RM) llamac
