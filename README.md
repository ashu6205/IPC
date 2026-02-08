This project implements a multi-client TCP server using a fixed-size thread pool on Linux, along with a simple TCP client.
The design avoids common concurrency issues like race conditions, excessive thread creation, and busy waiting.

The server uses:

Blocking sockets

Thread pool (worker threads)

Mutex + condition variables

Producer–consumer task queue

This architecture is widely used in real production servers.
