C = gcc
PHPSRC = /usr
PHPINC = $(PHPSRC)/include/php
PHPLIB = -L$(PHPSRC)/lib
CFLAGS = -c \
		 -I $(PHPINC) \
		 -I $(PHPINC)/main/ \
		 -I $(PHPINC)/Zend/ \
		 -I $(PHPINC)/TSRM/ \
		 -Wall -g -Wno-implicit-function-declaration \
		 -Wno-int-conversion
LDFLAGS = $(PHPLIB) -lphp7
all: main.c
	$(CC) -o main.o main.c $(CFLAGS)
	$(CC) -o rpc_server.o rpc_server.c $(CFLAGS)
	$(CC) -o phpcd main.o rpc_server.o $(LDFLAGS)

clean:
	rm -rf *.o
	rm -rf phpcd
