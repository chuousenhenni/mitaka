SRC=main.c queue.c app.c maintenance.c monitor.c common.c conta.c contb.c cont.c event.c io.c

SRC_MITAKA   = ./mitaka_src

OBJS=main.o queue.o app.o maintenance.o monitor.o common.o conta.o contb.o cont.o event.o io.o

PROG=main.exe

CC=gcc
CFLAGS=-Dwindows -Wall -O3
#LDFLAGS=-mwindows
#LDFLAGS=-lwsock32
RM=del


%.o: %.c define.h queue.h
	$(CC) -Dwindows $(CFLAGS) -o $@ -c $<

.PHONY : all

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $@

main.o: main.c
	$(CC) $(CFLAGS) -o main.o -c main.c

app.o: $(SRC_MITAKA)/app.c $(SRC_MITAKA)/define.h
	$(CC) $(CFLAGS) -o app.o -c $(SRC_MITAKA)/app.c

maintenance.o: $(SRC_MITAKA)/maintenance.c $(SRC_MITAKA)/define.h $(SRC_MITAKA)/version.h
	$(CC) $(CFLAGS) -o maintenance.o -c $(SRC_MITAKA)/maintenance.c

queue.o: $(SRC_MITAKA)/queue.c
	$(CC) $(CFLAGS) -o queue.o -c $(SRC_MITAKA)/queue.c

monitor.o: $(SRC_MITAKA)/monitor.c $(SRC_MITAKA)/define.h
	$(CC) $(CFLAGS) -o monitor.o -c $(SRC_MITAKA)/monitor.c

GPS.o: $(SRC_MITAKA)/GPS.c $(SRC_MITAKA)/define.h
	$(CC) $(CFLAGS) -o GPS.o -c $(SRC_MITAKA)/GPS.c

common.o: $(SRC_MITAKA)/common.c $(SRC_MITAKA)/define.h
	$(CC) $(CFLAGS) -o common.o -c $(SRC_MITAKA)/common.c

conta.o: $(SRC_MITAKA)/conta.c $(SRC_MITAKA)/define.h
	$(CC) $(CFLAGS) -o conta.o -c $(SRC_MITAKA)/conta.c

contb.o: $(SRC_MITAKA)/contb.c $(SRC_MITAKA)/define.h
	$(CC) $(CFLAGS) -o contb.o -c $(SRC_MITAKA)/contb.c

cont.o: $(SRC_MITAKA)/cont.c $(SRC_MITAKA)/define.h
	$(CC) $(CFLAGS) -o cont.o -c $(SRC_MITAKA)/cont.c

event.o: $(SRC_MITAKA)/event.c $(SRC_MITAKA)/define.h
	$(CC) $(CFLAGS) -o event.o -c $(SRC_MITAKA)/event.c

io.o: $(SRC_MITAKA)/io.c $(SRC_MITAKA)/define.h
	$(CC) $(CFLAGS) -o io.o -c $(SRC_MITAKA)/io.c

.PHONY : clean
clean:
	$(RM) *.o *.exe
