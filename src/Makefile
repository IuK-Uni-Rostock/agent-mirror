OBJECTS := agent.o sqlite.o flow.o list.o export.o telegram.o utils.o
CC = gcc
CFLAGS = -Wall -Wpedantic -std=gnu11 -march=native -O2 # -Wextra
TARGET = agent
LIBS = -lkdriveExpress -lpthread -lsqlite3

all: $(TARGET)

agent.o: agent.c
sqlite.o: sqlite.c sqlite.h
flow.o: flow.c flow.h
list.o: list.c list.h
export.o: export.c export.h
telegram.o: telegram.c telegram.h
utils.o: utils.c utils.h

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

clean:
	$(RM) $(OBJECTS) $(TARGET)
