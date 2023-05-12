CC=gcc
SRCS=adzip.c functions.c
OBJS=$(SRCS:.c=.o)
EXEC=adzip

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC)  $(OBJS) -o $@

%.o: %.c functions.h
	$(CC)  -c $< -o $@

clean:
	rm -f $(OBJS) $(EXEC)
