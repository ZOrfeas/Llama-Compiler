# **Llama language compiler**
## E.C.E. - N.T.U.A. Compilers class semester project
Implementation of a compiler for the functional programming language Llama

## Compile
`make` to create a production version
- `sudo apt install nasm` necessary for library compilation
- [libgc build from source](https://github.com/ivmai/bdwgc#installation-and-portability) or install for garbage collection (can be disabled in makefile)
- Bison build from source:
  ```wget http://ftp.gnu.org/gnu/bison/bison-3.7.6.tar.gz
     tar -zxvf bison-3.7.6.tar.gz
     cd bison-3.7.6/
     sudo ./configure
     sudo make
     sudo make install
  ```
  (possibly necessary) `sudo ln -s /usr/local/bin/bison /usr/bin/bison`
- `sudo apt install flex` should be enough
- LLVM installation is harder :)

## Built with
| Component           | Tools |
|   :---:             | :---: |
|   Lexer             | [Flex v2.6.4](https://github.com/westes/flex/releases)  |
|   Parser            | [Bison v3.7.6](https://www.gnu.org/software/bison/)     |
|   Semantic analysis | [C++11](https://en.cppreference.com/w/cpp/11)           |
|   Backend           | [LLVM v10.0.0](https://llvm.org/)                       |

## Authors
- [Jason Chatzitheodorou](https://github.com/JasonChatzitheodorou) (School ID: 03117089)
- [Orfeas Zografos](https://github.com/ZOrfeas) (School ID: 03117160)

## Dependencies
- [Assembly runtime library](https://github.com/abenetopoulos/edsger_lib/tree/master/)
- [Boehm-Demers-Weiser conservative C/C++ Garbage Collector](https://github.com/ivmai/bdwgc) (optional)

## Sources
- [Mapping High Level Constructs to LLVM IR](https://mapping-high-level-constructs-to-llvm-ir.readthedocs.io/en/latest/README.html#)
- [LLVM C++ API docs](https://llvm.org/doxygen/)
- [LLVM Language Reference Manual](https://releases.llvm.org/10.0.0/docs/LangRef.html)
- [Stack Overflow](https://stackoverflow.com/)
