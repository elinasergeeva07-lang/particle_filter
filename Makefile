CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -I./include -g
LDFLAGS = -lm

SRCDIR = src
INCDIR = include
OBJDIR = obj
BINDIR = bin

SOURCES = $(SRCDIR)/main.c $(SRCDIR)/particle_filter.c
OBJECTS = $(OBJDIR)/main.o $(OBJDIR)/particle_filter.o
TARGET = $(BINDIR)/particle_filter

.PHONY: all clean test dirs

all: dirs $(TARGET)

dirs:
	@mkdir -p $(OBJDIR) $(BINDIR)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

$(OBJDIR)/main.o: $(SRCDIR)/main.c $(INCDIR)/particle_filter.h
	$(CC) $(CFLAGS) -c $(SRCDIR)/main.c -o $(OBJDIR)/main.o

$(OBJDIR)/particle_filter.o: $(SRCDIR)/particle_filter.c $(INCDIR)/particle_filter.h
	$(CC) $(CFLAGS) -c $(SRCDIR)/particle_filter.c -o $(OBJDIR)/particle_filter.o

test: dirs
	@mkdir -p output
	$(CC) $(CFLAGS) $(SRCDIR)/tests.c $(SRCDIR)/particle_filter.c -o $(BINDIR)/tests $(LDFLAGS)
	./$(BINDIR)/tests

clean:
	rm -rf $(OBJDIR) $(BINDIR) output/*.csv