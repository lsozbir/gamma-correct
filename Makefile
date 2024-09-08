# Add additional compiler flags here
OPTL?=-O3
GDB?=-g
MATH = -lm
# WARNINGS = -Wall -Wextra -Wpedantic

all: main
main: main.c gamma_correct.c gamma_correct.h gamma_correct.S image_library.c image_library.h test.c test.h $(MATH)
	gcc $(OPTL) $(GDB) -o $@ $^
clean:
	rm -f main *.o *~

