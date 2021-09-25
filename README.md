# **Llama language compiler**
## E.C.E. - N.T.U.A. Compilers class semester project
Implementation of a compiler for the functional programming language Llama

## Compile
- `make` to create a production version

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
