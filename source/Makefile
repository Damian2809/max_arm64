# Compiler and flags
CC = aarch64-linux-gnu-gcc
CFLAGS = -Wall -O2 -I.
LDFLAGS = -lEGL -lGLESv2 -lopenal -ldl -lpthread -lm -lstdc++

# Source files
SRCS = config.c error.c imports.c main.c so_util.c util.c game.c openal.c opengl.c
OBJS = $(SRCS:.c=.o)

# Target executable
TARGET = maxpayne

# Rules
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
