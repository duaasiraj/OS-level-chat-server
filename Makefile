CC      = gcc
CFLAGS  = -Wall -Wextra -pthread -std=c99
LDFLAGS = -pthread

SERVER_SRCS = main.c server.c client.c threadpool.c \
              socket_util.c llist2.c log.c colors.c

CLIENT_SRCS = client_main.c socket_util.c log.c

SERVER_BIN  = chatserver
CLIENT_BIN  = chatclient

all: $(SERVER_BIN) $(CLIENT_BIN)

$(SERVER_BIN): $(SERVER_SRCS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(CLIENT_BIN): $(CLIENT_SRCS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

debug: CFLAGS += -g -O0
debug: all

clean:
	rm -f $(SERVER_BIN) $(CLIENT_BIN) *.o

.PHONY: all debug clean
