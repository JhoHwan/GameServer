# Instructions for Gemini
- **Continuous Documentation:** At the end of significant conversations or when important decisions, architectural notes, or analyses are made, summarize the key takeaways and append them to this file to keep a persistent record.

---

# Game Server Networking Architecture Notes

## ServerCore General Architecture
* **Core Philosophy:** High-performance, multi-platform asynchronous network engine using a Lock-Free JobQueue model.
* **Platform Abstraction:** Uses interfaces (`NetCore`, `NetObject`) and type aliasing (`NetEvent`) to separate business logic from OS-specific I/O (IOCP for Windows, Epoll for Linux).
* **Buffer Management:** Optimizes memory with `RecvBuffer` (ring buffer) and `SendBuffer` (chunk-based pooling) to minimize allocations and handle TCP fragmentation.

## Listener Pimpl Pattern & IOCP Lifecycle
The project uses a Pimpl (Pointer to Implementation) pattern for the `Listener` class to separate platform-independent socket logic from platform-specific asynchronous networking logic (e.g., Windows IOCP).

### Important Considerations for IOCP (Windows)
* **Pending I/O and Memory Management:** Never forcefully `delete` Overlapped event objects (like `AcceptEvent`) in the destructor if there are pending I/O operations. Doing so while the kernel is still processing causes memory corruption and server crashes.
* **Breaking Circular References:** Do not store `AcceptEvent` pointers in a container (like `vector`) within the `ListenerImpl` if the event also holds a `shared_ptr` to the `Listener`. This causes memory leaks. The lifecycle of `AcceptEvent` must be managed by the IOCP kernel queue.
* **Safe Cleanup (Graceful Shutdown):** 
  1. Call `CloseSocket()` to close the listen socket.
  2. The kernel aborts pending `AcceptEx` operations and returns them to the worker thread.
  3. The worker thread checks if the socket is `INVALID_SOCKET` and only then performs `delete acceptEvent`.
  4. This drops the reference count to 0, allowing the `Listener` to be safely destroyed.
* **Maintaining the Accept Pool:** If `AcceptEx` fails with an error other than `WSA_IO_PENDING`, but the listener is still active, the event must be recycled (`RegisterAccept`) to prevent the server from stalling due to an empty accept pool.

### Cross-Platform Build Requirements
* **Incomplete Types:** When using `unique_ptr` with a forward-declared implementation class, ensure that the full definition of the implementation class is available where the `unique_ptr` is destroyed (e.g., in the `Listener` destructor). Ensure `#ifdef _WIN32` blocks have corresponding `#else` blocks that include dummy classes or headers for other platforms (like Linux/Epoll) to prevent "incomplete type" compilation errors on non-Windows systems.

### Resource Management & Initialization
* **Socket Leaks:** In initialization functions like `StartAccept`, if any step fails after a socket has been created (e.g., binding, registering with the network core), ensure that `SocketUtils::Close()` is called to prevent resource leaks before returning false.
* **WinSock Initialization:** Always ensure `SocketUtils::Init()` (which calls `WSAStartup` and binds extension functions) is called before creating sockets, otherwise socket creation will fail (e.g., resulting in error 87 `ERROR_INVALID_PARAMETER` when registering with IOCP). This was fixed in `ClientService::Start()`.

## Testing the Network Engine
* **Dummy Projects:** The repository includes `DummyServer` and `DummyClient` projects under the `Server` directory. These are standalone CMake executables designed purely to stress-test the `ServerCore` networking logic (Accept pools, IOCP threading, graceful shutdown) without the overhead of game logic.
* **Test Strategy:** Use `ClientService` with a high `maxSessionCount` to simulate hundreds of concurrent connections to the `ServerService` to verify thread safety and memory leak prevention during rapid connect/disconnect cycles.
