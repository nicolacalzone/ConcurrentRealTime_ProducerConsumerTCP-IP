PROGRAMS = TcpClient \
           TcpThreadedServer \
           prodcons_threaded

COMPILED_PROGRAMS = $(addsuffix _compiled, $(PROGRAMS))

CFLAGS = -Iinclude

all: $(COMPILED_PROGRAMS)

TcpThreadedServer: LDLIBS += -lpthread
prodcons_threaded: LDLIBS += -lpthread

%_compiled: %.c
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

clean:
	$(RM) $(PROGRAMS)
