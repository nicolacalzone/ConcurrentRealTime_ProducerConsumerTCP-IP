PROGRAMS = TcpClient \
           TcpServer \
           prodConMonitor

COMPILED_PROGRAMS = $(addsuffix _executable, $(PROGRAMS))

CFLAGS = -Iinclude

all: $(COMPILED_PROGRAMS)

TcpThreadedServer: LDLIBS += -lpthread
prodcons_threaded: LDLIBS += -lpthread

%_executable: %.c
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

clean:
	$(RM) $(COMPILED_PROGRAMS)
