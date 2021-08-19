.PHONY: default clean distclean

CXX=c++
CXXFLAGS=-Wall -std=c++11 `llvm-config --cxxflags`
LDFLAGS=`llvm-config --ldflags --system-libs --libs all`

default: compiler

lexer.cpp: lexer.l
	flex -s -o lexer.cpp lexer.l

parser.hpp parser.cpp: parser.y
	bison -dv --debug -Wall -o parser.cpp parser.y

# Add your dependencies here, e.g.
# parser.o: parser.cpp lexer.hpp symbol.hpp
lexer.o: lexer.cpp lexer.hpp parser.hpp ast.hpp
parser.o: parser.cpp lexer.hpp symbol.hpp ast.hpp infer.hpp
ast.o: ast.cpp lexer.hpp ast.hpp parser.hpp types.hpp infer.hpp
symbol.o: symbol.cpp symbol.hpp
types.o: types.cpp types.hpp
infer.o: infer.cpp infer.hpp types.hpp

# compiler: lexer.o parser.o symbol.o
compiler: lexer.o parser.o symbol.o types.o symbol.o ast.o infer.o
	$(CXX) $(CXXFLAGS) -o llamac $^ $(LDFLAGS)

clean:
	$(RM) lexer.cpp parser.cpp parser.hpp parser.output *.o

distclean: clean
	$(RM) llamac llamac_d

#### Build for debugging 
debug: CXXFLAGS+=-g

lexer_d.o: lexer.cpp lexer.hpp parser.hpp ast.hpp
	$(CXX) $(CXXFLAGS) -c -o lexer_d.o lexer.cpp
parser_d.o: parser.cpp lexer.hpp symbol.hpp ast.hpp infer.hpp
	$(CXX) $(CXXFLAGS) -c -o parser_d.o parser.cpp
ast_d.o: ast.cpp lexer.hpp ast.hpp parser.hpp types.hpp infer.hpp
	$(CXX) $(CXXFLAGS) -c -o ast_d.o ast.cpp
symbol_d.o: symbol.cpp symbol.hpp types.hpp
	$(CXX) $(CXXFLAGS) -c -o symbol_d.o symbol.cpp
types_d.o: types.cpp types.hpp infer.hpp
	$(CXX) $(CXXFLAGS) -c -o types_d.o types.cpp
infer_d.o: infer.cpp infer.hpp types.hpp
	$(CXX) $(CXXFLAGS) -c -o infer_d.o infer.cpp

debug: lexer_d.o parser_d.o ast_d.o symbol_d.o types_d.o infer_d.o
	$(CXX) $(CXXFLAGS) -o llamac_d lexer_d.o parser_d.o symbol_d.o types_d.o ast_d.o infer_d.o

