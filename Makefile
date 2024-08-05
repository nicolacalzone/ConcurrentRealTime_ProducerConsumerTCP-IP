PROGRAMS = TcpClient \
           TcpThreadedServer \
           prodcons_msgqueue

all: $(PROGRAMS)

TcpClient_compiled: TcpClient.c
    gcc -o TcpClient_compiled TcpClient.c
TcpThreadedServer_compiled: TcpThreadedServer.c -lpthread
    gcc -o TcpThreadedServer_compiled TcpThreadedServer.c -lpthread
prodcons_msgqueue_compiled: prodcons_msgqueue.c -lpthread
    gcc -o prodcons_msgqueue_compiled prodcons_msgqueue.c -lpthread

clean:
    rm -f $(PROGRAMS)
