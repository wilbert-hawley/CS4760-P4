MASTER_C = oss.c
MASTER_O = oss.o
MASTER = oss
CHILD_C = child_proc.c
CHILD_O = child_proc.o
CHILD = child_proc
UTILITY_H = utility.h
UTILITY_O = utility.o

CC = gcc
CFLAGS = -Wall -g

all: $(MASTER) $(CHILD)

$(MASTER): $(MASTER_O) $(UTILITY_O)
	$(CC) $(CFLAGS) $(MASTER_O) $(UTILITY_O) -o $(MASTER)

$(CHILD): $(CHILD_O) $(UTILITY_O)
	$(CC) $(CFLAGS) $(CHILD_O) $(UTILITY_O) -o $(CHILD)

%.o: %.c $(UTILITY_H)
	$(CC) $(CFLAGS) -c $*.c -o $*.o

.PHONY: clean
clean:
	/bin/rm -f $(MASTER) $(CHILD) *.o logfile
