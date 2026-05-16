# 1. Compiler and Flags
CC = gcc

CFLAGS = -Wall -Wextra -Wpedantic -g -O2 -I/opt/homebrew/include

# 2. Linker Flags (Libraries)

LDFLAGS = -pthread -L/opt/homebrew/lib -lgsl -lgslcblas -lm

# 3. Target Executable Name
TARGET = qsim

# 4. Files
SRCS = main.c engine.c parser.c qmath.c types.c
OBJS = $(SRCS:.c=.o)
DEPS = engine.h parser.h qmath.h types.h


# Default target: build the executable
.PHONY: all
all: $(TARGET)

# Link the object files to create the executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Compile individual source files into object files
# $< is the first dependency (.c file), $@ is the target (.o file)
%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

# Clean rule to remove compiled files and the executable
.PHONY: clean
clean:
	rm -f $(OBJS) $(TARGET)

# Rule to run the program quickly with a test case (optional)
.PHONY: run
run: $(TARGET)
	./$(TARGET) -s state.txt -c circuit.txt -t 4