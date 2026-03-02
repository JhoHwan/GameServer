# Instructions for Gemini
- **Continuous Documentation:** At the end of significant conversations or when important decisions, architectural notes, or analyses are made, summarize the key takeaways and append them to this file to keep a persistent record.

---

# Game Server Networking Architecture Notes

## ServerCore General Architecture
* **Core Philosophy:** High-performance, multi-platform asynchronous network engine using a Lock-Free JobQueue model.
* **Platform Abstraction:** Uses interfaces (`NetCore`, `NetObject`) and platform-specific implementations. The project prioritizes runtime performance (CPU cache efficiency) by avoiding Pimpl patterns in hot paths to minimize pointer chasing and cache misses.
* **Conditional Compilation:** Platform-specific data members and logic are handled via `#ifdef _WIN32` blocks and CMake-based source file separation (`Platform/Windows` vs `Platform/Linux`).

## Windows IOCP Lifecycle and Safety
* **Pending I/O and Memory Management:** Never forcefully delete Overlapped event objects (like `AcceptEvent`) while I/O operations are pending. The kernel must return the event to the worker thread before safe deletion.
* **Safe Cleanup:** Close the socket first to abort pending operations. Only delete the event object in the worker thread after verifying the socket is `INVALID_SOCKET`.
* **Maintaining Accept Pool:** If `AcceptEx` fails with an error other than `WSA_IO_PENDING`, the event must be recycled immediately to prevent the accept pool from exhausting.

## Linux Epoll Implementation Notes
* **Reactor Model:** Unlike IOCP's Proactor model, Epoll notifies when a socket is ready for I/O. Implementation must handle `EAGAIN` or `EWOULDBLOCK` by looping `recv`, `send`, or `accept` until the buffer is exhausted (Edge Triggered mode).
* **Lifecycle Management (_epollRef):** To prevent dangling pointers during multi-threaded dispatch, `NetObject` maintains a `shared_ptr` to itself (`_epollRef`) while registered in Epoll. This reference is acquired during `Register` and released upon `Disconnect` or `Close`.
* **Event Structure:** `EpollEvent` is a lightweight wrapper for kernel flags. Specialized events like `SendEvent` act as local buffers to hold `SendBufferRef` objects for data that couldn't be fully sent in a single system call.

## Build Environment and Encoding
* **Encoding (UTF-8 without BOM):** All source and header files must be saved in UTF-8 without BOM to ensure compatibility with GCC/WSL. BOM markers can cause GCC to ignore `#pragma once`, leading to massive redefinition errors.
* **MSVC Configuration:** To support BOM-less UTF-8 and clean build output in CLion, the project uses `/utf-8` and `/wd4819` compiler flags, along with `VSLANG=1033` for English error messages.
* **PCH Management:** Manual inclusion of `pch.h` in `.cpp` files should be avoided in `ServerCore` to prevent conflicts with CMake's automatic PCH injection (`target_precompile_headers`).

## Resource Management
* **Socket Leaks:** Sockets must be closed using `SocketUtils::Close()` immediately if any subsequent initialization step (binding, registering) fails.
* **WinSock Initialization:** `SocketUtils::Init()` must be called before any socket operations to ensure extension functions (ConnectEx, AcceptEx) are bound.

## Testing
* **Dummy Projects:** Use `DummyServer` and `DummyClient` for stress-testing basic networking stability, thread safety, and memory leak prevention under high concurrency.
