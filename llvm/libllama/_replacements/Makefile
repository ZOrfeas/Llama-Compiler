
lib: libcustom.o
	ar -cvqs reps.a libcustom.o

libcustom.o: lib.c
	gcc -std=c11 -O3 -fno-stack-protector -c -o libcustom.o lib.c

clean:
	rm *.o