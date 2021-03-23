.PHONY: default clean

CXX=c++
CXXFLAGS=-Wall -std=c++11

default: llexer

llexer.cpp: lexer.l
	flex -s -o llexer.cpp lexer.l

llexer: llexer.cpp
	$(CXX) $(CXXFLAGS) -o llexer llexer.cpp

clean:
	$(RM) llexer.cpp

distclean: clean
	$(RM) llexer