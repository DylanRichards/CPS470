LD = gcc
LDFLAGS =
CC = gcc
CFLAGS  = -g -Wformat-security -Wall -Wextra -Werror -ansi

TARG = shell
OBJS = $(TARG).o

$(TARG): $(OBJS)
	$(LD) $(OBJS) $(LDFLAGS) -o $(TARG)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	$(RM) *.o $(TARG)