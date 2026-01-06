CC = clang
CFLAGS = -std=c99 -Wall
CPPFLAGS += -I/opt/homebrew/opt/libedit/include
LDFLAGS  += -L/opt/homebrew/opt/libedit/lib
LDLIBS   = -ledit

TARGET = prompt
SRCS = src/prompt.c

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(SRCS) $(LDFLAGS) $(LDLIBS) -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)

