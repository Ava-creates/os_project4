CC=gcc
SRCS=final.c functions.c
OBJS=$(SRCS:.c=.o)
EXEC=final

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC)  $(OBJS) -o $@

%.o: %.c functions.h
	$(CC)  -c $< -o $@

clean:
	rm -f $(OBJS) $(EXEC)
