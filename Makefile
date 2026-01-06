CC = clang
CFLAGS = -std=c99 -Wall
CPPFLAGS += -I/opt/homebrew/opt/libedit/include
LDFLAGS  += -L/opt/homebrew/opt/libedit/lib
LDLIBS   = -ledit

TARGET = parsing
SRCS = src/parsing.c src/mpc.c


all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(SRCS) $(LDFLAGS) $(LDLIBS) -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
