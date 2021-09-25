.PHONY: default clean distclean 

CXX=c++
CXXFLAGS=-Wall -std=c++11 `llvm-config --cxxflags`
LDFLAGS=`llvm-config --ldflags --system-libs --libs all`

LIBGCFLAG=-DLIBGC="/usr/local/lib/libgc.so"
# LIBGCFLAG=
LIBLLAMAFLAG=-DLIBLLAMA="./libllama/lib.a"
GENIRCPPFLAGS=$(CXXFLAGS) $(LIBGCFLAG)
OPTIONCPPFLAGS=$(CXXFLAGS) $(LIBLLAMAFLAG) $(LIBGCFLAG)

default: all

lexer.cpp: lexer.l
	flex -s -o lexer.cpp lexer.l

parser.hpp parser.cpp: parser.y
	bison -dv --debug -Wall -o parser.cpp parser.y

# Add your dependencies here, e.g.
# parser.o: parser.cpp lexer.hpp symbol.hpp
lexer.o: lexer.cpp lexer.hpp parser.hpp ast.hpp
parser.o: parser.cpp lexer.hpp ast.hpp parser.hpp options.hpp
ast.o: ast.cpp lexer.hpp ast.hpp
printOn.o: printOn.cpp ast.hpp parser.hpp
sem.o: sem.cpp ast.hpp parser.hpp  
symbol.o: symbol.cpp symbol.hpp
types.o: types.cpp types.hpp ast.hpp
infer.o: infer.cpp infer.hpp types.hpp
liveness.o: liveness.cpp ast.hpp
genIR.o: genIR.cpp ast.hpp infer.hpp parser.hpp
	$(CXX) $(GENIRCPPFLAGS) -c -o genIR.o genIR.cpp $(LDFLAGS)
libIR.o: libIR.cpp ast.hpp
options.o: options.cpp ast.hpp infer.hpp symbol.hpp
	$(CXX) $(OPTIONCPPFLAGS) -c -o options.o options.cpp $(LDFLAGS)

# compiler: lexer.o parser.o symbol.o
all: lib compiler
compiler: lexer.o parser.o symbol.o types.o ast.o printOn.o sem.o infer.o libIR.o liveness.o genIR.o options.o
	$(CXX) $(CXXFLAGS) -o llamac $^ $(LDFLAGS)

lib:
	cd libllama && ./libs.sh &&	cd ..

clean:
	$(RM) lexer.cpp parser.cpp parser.hpp parser.output *.o

distcleanlib:
	cd libllama && \
	make -s clean -C auxil  && \
	rm auxil/auxil.a && \
	make -s clean -C math && \
	rm math/math.a && \
	make -s clean -C stdio && \
	rm stdio/stdio.a && \
	make -s clean -C stdlib && \
	rm stdlib/stdlib.a && \
	make -s clean -C string && \
	rm string/string.a && \
	make -s clean -C _replacements && \
	rm _replacements/reps.a && \
	rm lib.a && \
	cd ..


distclean: clean distcleanlib
	$(RM) llamac
