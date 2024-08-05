PROGRAMS = TcpClient \
           TcpThreadedServer \
           prodcons_msgqueue

COMPILED_PROGRAMS = $(addsuffix _compiled, $(PROGRAMS))

all: $(COMPILED_PROGRAMS)

TcpThreadedServer: LDLIBS += -lpthread
prodcons_msgqueue: LDLIBS += -lpthread

%_compiled: %.c
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

# Clean target
clean:
	$(RM) $(PROGRAMS)
