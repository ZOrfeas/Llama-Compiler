SRC=$(wildcard *.c)
OBJ=$(SRC:.c=.o)

lib: $(OBJ)

%.o : %.c
	$(CC) -c -o $@ $<

clean :
	$(RM) *.o
