# **Llama language compiler** <!-- omit in toc -->
## E.C.E. - N.T.U.A. Compilers class semester project <!-- omit in toc -->
Implementation of a compiler for the functional programming language Llama

- [Supported features](#supported-features)
  - [Example commands](#example-commands)
- [Build](#build)
- [Built with](#built-with)
- [Authors](#authors)
- [Other Dependencies](#other-dependencies)
- [Sources](#sources)

## Supported features
most if not all of what is specified [here](https://courses.softlab.ntua.gr/compilers/2021a/llama2021.pdf)  
some worth mentioning are:
- User defined types
- Type inference
- Function closures
- Garbage collection
- Floating point arithmetic
- Optimizations
- Modular execution

### Example commands
```bash
./llamac [llama-source-file] # optionally can add -o [executable-file-name] 
./llamac [llama-source-file] -i -o [llvm-IR-file-name] -frontend compile # produces IR source file
./llamac [llama-source-file] -S -o [assembly-file-name] -frontend compile # produces assembly source file
```

## Build
`make` to create a production version
<details>
  <summary>Dependency installation tips</summary>

- `sudo apt install nasm` necessary for library compilation
- [libgc build from source](https://github.com/ivmai/bdwgc#installation-and-portability) for garbage collection (can be disabled in makefile)
  Build summary:
  ```bash
  git clone git://github.com/ivmai/bdwgc.git
  cd bdwgc
  git clone git://github.com/ivmai/libatomic_ops.git
  ./autogen.sh
  ./configure
  make -j
  make check
  make install
  ```

- Bison build from source:
  ```bash
  wget http://ftp.gnu.org/gnu/bison/bison-3.7.6.tar.gz
  tar -zxvf bison-3.7.6.tar.gz
  cd bison-3.7.6/
  sudo ./configure
  sudo make
  sudo make install
  # sudo ln -s /usr/local/bin/bison /usr/bin/bison # (possibly necessary)
  ```
- `sudo apt install flex` should be enough
- LLVM Install precompiled binaries [source](https://releases.llvm.org/download.html)
  ```bash
  wget [proper-tarball-url]
  sudo tar -xf [tarbal name].tar.xz --strip-components=1 -C [install-dir]
  export PATH=$PATH:[install-dir]/bin
  cd [install-dir]
  sudo echo [install-dir]/lib >> /etc/ld.so.conf.d/libs.conf # make sure it's in a new line
  sudo ldconfig
  ```
  check installation with `llvm-config --version`
- Build garbage collector from source [more docs](https://bestofcpp.com/repo/ivmai-bdwgc-cpp-memory-allocation#installation-and-portability)
 </details>
  
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

## Other Dependencies
- [Assembly runtime library](https://github.com/abenetopoulos/edsger_lib/tree/master/) (altered and bundled in libllama directory)
- [Boehm-Demers-Weiser conservative C/C++ Garbage Collector](https://github.com/ivmai/bdwgc) (optional)

## Sources
- [Mapping High Level Constructs to LLVM IR](https://mapping-high-level-constructs-to-llvm-ir.readthedocs.io/en/latest/README.html#)
- [LLVM C++ API docs](https://llvm.org/doxygen/)
- [LLVM Language Reference Manual](https://releases.llvm.org/10.0.0/docs/LangRef.html)
- [Stack Overflow](https://stackoverflow.com/)
