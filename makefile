# Compiler
CC = gcc

# Source file
SRC = star.c

# Output file
OUT = star

# Just to compile
all:
	$(CC) $(SRC) -o $(OUT)

clean:
	rm -f $(OUT) 

