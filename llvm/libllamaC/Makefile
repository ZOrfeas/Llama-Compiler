MAKECLEANDIR=$(MAKE) -s clean -C
MAKELIBDIR=$(MAKE) -s lib -C
LIBNAME=lib.a
lib: math stdio stdlib
	$(MAKELIBDIR) math
	$(MAKELIBDIR) stdio
	$(MAKELIBDIR) stdlib
	ar -cvqs $(LIBNAME) math/*.o stdio/*.o stdlib/*.o

distclean: clean
	$(RM) $(LIBNAME)

clean:
	$(MAKECLEANDIR) math
	$(MAKECLEANDIR) stdio
	$(MAKECLEANDIR) stdlib
