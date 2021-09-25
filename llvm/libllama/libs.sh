make -s lib -C auxil
make -s lib -C math
make -s lib -C stdio
make -s lib -C stdlib
make -s lib -C string
make -s lib -C _replacements

ar -cvqs lib.a auxil/*.o math/*.o \
         stdio/*.o stdlib/*.o string/*.o \
         _replacements/*.o
objcopy --redefine-syms=change_syms lib.a

# make -s clean -C auxil
# rm auxil/auxil.a
# make -s clean -C math
# rm math/math.a
# make -s clean -C stdio
# rm stdio/stdio.a
# make -s clean -C stdlib
# rm stdlib/stdlib.a
# make -s clean -C string
# rm string/string.a
# make -s clean -C _replacements
# rm _replacements/reps.a
