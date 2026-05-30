# OS Level Chat Server

A multi-client TCP chat server written in C, built from the ground up using OS-level primitives. The server manages concurrent connections through a custom thread pool, tracks connected clients in a mutex-guarded linked list, and supports a set of in-chat commands delivered over raw sockets. A minimal terminal client is included for connecting directly without an external tool like telnet.

---

## Table of Contents

- [Overview](#overview)
- [Architecture](#architecture)
- [Features](#features)
- [Requirements](#requirements)
- [Building](#building)
- [Usage](#usage)
  - [Server](#server)
  - [Client](#client)
- [Chat Commands](#chat-commands)
- [Source Layout](#source-layout)
- [Design Notes](#design-notes)
- [Acknowledgements](#acknowledgements)

---

## Overview

The project is split into two binaries: `chatserver` and `chatclient`. The server listens for incoming TCP connections, assigns each connection to a worker thread from a pre-allocated pool, and routes messages between clients. The client binary provides a simple two-threaded terminal interface that sends typed input while concurrently printing messages arriving from the server.

---

## Architecture

```
                        +------------------+
                        |   chatserver     |
                        |                  |
  TCP connections  -->  |  server_run()    |
                        |   accept loop    |
                        +--------+---------+
                                 |
                         registry_add(fd)
                         tp_enqueue(pool, fd)
                                 |
                    +------------+------------+
                    |       threadpool_t       |
                    |  circular queue, 8 workers|
                    +---+----+----+----+-------+
                        |    |    |    |
                   handle_client() per fd
                        |
               process_msg() -> broadcast()
                              -> send_private()
```

Each accepted file descriptor is registered in the client registry (a linked list) and pushed onto the thread pool's task queue. A free worker thread picks it up and drives the entire lifecycle for that connection: authentication, message processing, and cleanup on disconnect.

---

## Features

- Concurrent client handling via a fixed-size thread pool (8 workers, configurable)
- Per-node mutex locking on the client linked list to minimise contention
- Username registration on connect with duplicate-name rejection
- Broadcast messaging to all connected clients
- Private messaging between named users
- In-chat commands: `/nick`, `/pm`, `/who`, `/me`, `/quit`
- Rate limiting: clients are limited to 5 messages per second
- UTF-8 validation on all incoming messages, allowing emoji and non-ASCII input
- ANSI terminal colours for join/leave events, private messages, and actions
- Configurable port and log level via command-line flags
- Graceful shutdown on `SIGINT` / `SIGTERM` with final broadcast to connected clients
- `SO_REUSEADDR` set on the server socket to allow immediate restart after a crash

---

## Requirements

- Linux (the server uses POSIX threads and BSD socket APIs)
- GCC with C11 support (`-std=gnu11`)
- `pthread` library (included in most Linux distributions)

No external dependencies are required.

---

## Building

```bash
make
```

This produces two binaries in the current directory:

| Binary       | Description                  |
|--------------|------------------------------|
| `chatserver` | The multi-client chat server |
| `chatclient` | The terminal chat client     |

To remove compiled binaries:

```bash
make clean
```

---

## Usage

### Server

```
./chatserver [-p port] [-l loglevel] [-h]
```

| Flag        | Description                                      | Default |
|-------------|--------------------------------------------------|---------|
| `-p port`   | TCP port to listen on                            | `5000`  |
| `-l level`  | Log verbosity: `1` = ERROR, `2` = INFO, `3` = DEBUG | `2` |
| `-h`        | Print usage and exit                             |         |

Example:

```bash
./chatserver -p 6000 -l 3
```

The server binds to all interfaces (`0.0.0.0`) by default. To change the bind address, set the `BIND_IP` preprocessor define at compile time.

Clients may also connect using any raw TCP client, such as telnet or netcat:

```bash
telnet 127.0.0.1 5000
nc 127.0.0.1 5000
```

### Client

```
./chatclient [ip] [port]
```

| Argument | Description         | Default     |
|----------|---------------------|-------------|
| `ip`     | Server IP address   | `127.0.0.1` |
| `port`   | Server port         | `5000`      |

Example:

```bash
./chatclient 192.168.1.10 6000
```

On connection, the server prompts for a username. Once a unique name is submitted, the client enters the chat room. The client uses two threads: one reads from stdin and sends to the server; the other continuously receives and prints server messages.

Type `exit` or press `Ctrl+D` to disconnect.

---

## Chat Commands

Commands are entered in the chat input and begin with `/`.

| Command             | Description                                      |
|---------------------|--------------------------------------------------|
| `/nick <name>`      | Change your display name to `<name>`             |
| `/pm <nick> <msg>`  | Send a private message to user `<nick>`          |
| `/who`              | List all currently connected users               |
| `/me <text>`        | Broadcast an action message in third-person      |
| `/quit`             | Disconnect from the server                       |

Any input beginning with `/` that does not match a known command returns a usage hint.

---

## Source Layout

```
.
├── main.c            Entry point; parses CLI flags, initialises subsystems, starts server loop
├── server.c / .h     Socket setup, accept loop, signal handling, graceful shutdown
├── client.c / .h     Client registry, connection handler, message processor, rate limiter
├── client_main.c     Standalone terminal client binary
├── threadpool.c / .h Fixed-size worker thread pool with circular task queue
├── llist.c           Mutex-per-node linked list implementation (client registry storage)
├── llist2.h          client_info and list_entry struct definitions
├── socket_util.c / .h  IPv4 TCP socket creation helpers
├── log.c / .h        Thread-safe timestamped logger with configurable verbosity
├── colors.c / .h     ANSI escape code definitions for terminal colour output
├── bool.h            Portable boolean type definition
└── Makefile
```

---

## Design Notes

**Thread pool.** Rather than spawning a new thread per connection, a fixed pool of 8 worker threads is created at startup. Accepted file descriptors are pushed onto a circular queue protected by a mutex and a condition variable. Workers block on `pthread_cond_wait` when the queue is empty, waking only when a new connection is enqueued. This bounds resource usage regardless of connection rate.

**Client registry.** Connected clients are stored in a singly-linked list. Each node carries its own `pthread_mutex_t`, so concurrent threads can traverse or modify different nodes simultaneously without locking the entire list. A separate coarse-grained mutex protects structural changes (insert, full traversal for broadcast).

**Rate limiting.** Each `client_info` struct stores a message counter and a timestamp. On every incoming message the handler checks whether more than `RATE_LIMIT_COUNT` messages (5) have arrived within a one-second window. Clients that exceed the limit receive a warning and their message is dropped for that window.

**UTF-8 validation.** All incoming message bytes are validated against the UTF-8 encoding rules before processing. This prevents malformed byte sequences from reaching other clients while still permitting full Unicode input including multi-byte characters and emoji.

**Graceful shutdown.** `SIGINT` and `SIGTERM` both invoke `server_shutdown()`, which broadcasts a shutdown notice, drains and destroys the thread pool, and closes the listening socket before exiting. `SIGPIPE` is ignored so that a write to a closed client socket does not kill the server process.

---

## Acknowledgements

This project was built with reference to **chatsrv** by André Gasser (version 0.5, 2012), a multi-threaded chat server written in C that served as an early example of this class of program. The implementation here is written independently, with different data structures, a thread pool architecture, and additional features, but credit is due to chatsrv as a reference point and source of inspiration.

The chatsrv source is available at: [(https://github.com/andregasser/chatsrv)](https://github.com/andregasser/chatsrv)
