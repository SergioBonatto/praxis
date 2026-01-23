CC = clang
CFLAGS = -std=c99 -Wall -I/opt/homebrew/opt/libedit/include
LDFLAGS = -L/opt/homebrew/opt/libedit/lib -ledit

TARGET = praxis

SRCS = src/parsing.c src/mpc.c
OBJS = $(SRCS:.c=.o)

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

run: all
	./$(TARGET)

clean:
	rm -f $(TARGET) $(OBJS)
