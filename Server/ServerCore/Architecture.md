# ServerCore Architecture & Implementation Guide

이 문서는 게임 서버의 핵심 네트워크 엔진인 `ServerCore` 프로젝트의 전반적인 구조와 구현 방식, 그리고 각 폴더와 파일이 수행하는 역할을 상세하게 정리한 기술 문서입니다. 현재 프로젝트는 Windows(IOCP)와 Linux(Epoll)를 모두 지원하기 위한 멀티 플랫폼 추상화가 적용되어 있습니다.

## 1. 프로젝트 전체 구성 및 핵심 원리

`ServerCore`는 대규모 동시 접속 환경에서 병목 현상을 최소화하고 고성능 처리를 하기 위해 설계된 비동기 네트워크/스레딩 라이브러리입니다.

### 핵심 설계 철학
1. **멀티 플랫폼 비동기 I/O (인터페이스 분리)**: Windows의 IOCP와 Linux의 Epoll을 공통 인터페이스(`NetCore`)로 추상화하였습니다. 비즈니스 로직(상위 레이어)은 OS에 종속되지 않고 `NetCore`, `NetObject`, `NetEvent`와 같은 공통 추상화 레이어를 통해 통신합니다.
2. **Lock-Free 지향 및 JobQueue 모델**: 멀티스레드 환경에서 자원 접근 시 발생하는 락(Lock) 경합(Contention)을 줄이기 위해, 객체별 순차 실행을 보장하는 `JobQueue`를 도입했습니다.
3. **조건부 컴파일 및 소스 분리**: 플랫폼 종속적인 로직은 클래스 내부의 `#ifdef _WIN32` 매크로와 CMake 레벨에서의 소스 파일 분리(`Platform/Windows` vs `Platform/Linux`)를 통해 구현됩니다. 단일 `NetCore` 클래스의 선언(Header)은 공유하되, 내부 구현(Cpp)은 플랫폼별로 나뉘어 빌드됩니다.
4. **메모리 및 버퍼 최적화**: 잦은 동적 할당/해제로 인한 성능 저하를 막기 위해, 스레드 로컬 저장소(TLS)와 연속적인 링 버퍼(`RecvBuffer`), Chunk 기반 풀링(`SendBuffer`) 등을 적극 활용합니다.

---

## 2. 폴더별 상세 구현 및 파일의 역할

프로젝트는 역할에 따라 다음과 같이 폴더로 나뉘어 있습니다.

### Network (공통 네트워크 인터페이스 및 로직)
플랫폼에 관계없이 공통으로 사용되는 네트워크 추상화 레이어입니다.

* **`NetCore.h`**: 네트워크 엔진의 최상위 인터페이스(`Dispatch`, `Register`, `Update` 등) 선언.
* **`NetObject.h / .cpp`**: 네트워크 엔진에 등록되는 모든 통신 객체(`Listener`, `Session`)의 최상위 추상 클래스. OS 이벤트를 수신하는 `Dispatch` 가상 함수 제공.
* **`NetEvent.h / .cpp`**: 플랫폼별 이벤트 객체(`IocpEvent`, `EpollEvent`)를 `#ifdef _WIN32`를 통해 추상화한 클래스.
* **`Session.h / .cpp`**: `NetObject`를 상속받아 실제 통신 로직(Send/Recv)을 수행. 플랫폼에 따라 다른 멤버 변수(`IocpEvent` vs `_sendOffset`)를 보유.
* **`Listener.h / .cpp`**: `NetObject`를 상속받아 서버의 접속 대기(Accept) 로직을 담당.
* **`Service.h / .cpp`**: `NetCore` 엔진의 인스턴스를 소유하고, `Listener`와 다수의 `Session`들의 생성 및 생명주기를 관리하는 핵심 관리자.
* **`SocketUtils.h / .cpp`**, **`NetAddress.h / .cpp`**: 소켓 설정, 주소 바인딩, 초기화 유틸리티.

### Platform (플랫폼 전용 구현체)
운영체제별 비동기 I/O 모델을 처리하는 폴더입니다. `NetCore.h`의 실제 구현을 담고 있습니다.

* **Windows (`Platform/Windows`)**
  * **`NetCore.cpp`**: IOCP 기반의 `NetCore` 구현체. `CreateIoCompletionPort`와 `GetQueuedCompletionStatus`를 사용하여 이벤트 처리.
  * **`IocpEvent.h / .cpp`**: Windows의 `OVERLAPPED` 구조체를 상속받아 완료 통지를 처리하기 위한 이벤트.
* **Linux (`Platform/Linux`)**
  * **`NetCore.cpp`**: Epoll 기반의 `NetCore` 구현체. `epoll_ctl`과 `epoll_wait` (Edge-Triggered, OneShot) 활용.
  * **`EpollEvent.h / .cpp`**: Linux 환경에서 Epoll 이벤트 플래그(`eventFlags`)를 담기 위한 가벼운 구조체.

### Buffer (메모리 및 스트림 관리)
* **`RecvBuffer.h / .cpp`**: TCP 파편화(Fragmentation) 문제를 해결하는 링 버퍼.
* **`SendBuffer.h / .cpp`**: 송신 데이터를 담는 메모리 버퍼. 단편화를 최소화하도록 Chunk 형태로 설계됨.

### Thread (멀티스레드 및 동기화)
* **`JobQueue.h / .cpp`, `Job.h / .cpp`**: 작업을 큐에 쌓아 순차적으로 실행함으로써 락 경합을 방지하는 핵심 모듈.
* **`JobTimer.h / .cpp`**: 예약된 시간에 작업을 실행하는 타이머.
* **`concurrentqueue.h`**: 스레드 안전한 락프리 큐 라이브러리 (Third-party).

### Main (공통 설정 및 유틸리티)
* **`Types.h`, `CoreMacro.h`, `Singleton.h`**: 고정 크기 자료형, 스마트 포인터(`Ref`), 공통 매크로 정의.
* **`CorePch.h / .cpp` & `pch.h`**: 미리 컴파일된 헤더(PCH).
* **`CoreTLS.h / .cpp`**: 스레드 로컬 스토리지(TLS) 전역 변수 관리 (예: `SendBufferChunk`, `LJobQueue`).

---

## 3. 플랫폼별 비동기 I/O 모델 (NetCore)

### Windows (IOCP - Proactor 패턴)
* **동작 방식:** 비동기 I/O 작업(예: `WSARecv`)을 커널에 요청하고, 작업이 완료되면 커널이 완료 큐(Completion Port)에 이벤트를 넣습니다. `GetQueuedCompletionStatus`를 통해 완료된 작업을 꺼내옵니다.
* **이벤트 처리:** `IocpEvent`(즉 `OVERLAPPED` 상속 객체)가 I/O의 문맥을 담고 있으며, 완료 통지 시 이 이벤트 객체 내의 `EventType`을 통해 어떤 작업이 완료되었는지 확인합니다.

### Linux (Epoll - Reactor 패턴)
* **동작 방식:** `epoll_wait`를 호출하여 소켓에서 I/O를 수행할 수 있는 "준비된(Ready)" 상태가 될 때 커널로부터 통지를 받습니다.
* **Edge-Triggered (EPOLLET) & EPOLLONESHOT:** 높은 성능을 위해 상태가 변할 때만 알림을 주는 Edge-Triggered 모드를 사용하며 `EAGAIN` 발생 시까지 루프를 돕니다. 멀티스레드 환경에서 한 소켓의 이벤트가 분산 처리되는 것을 막기 위해 `EPOLLONESHOT`을 설정하며, 이벤트 처리 후에는 `NetCore::Update()`로 소켓을 재등록(re-arm)해야 합니다.

---

## 4. 객체 생명주기 및 안정성 관리 (Lifecycle & Safety)

### Windows: 펜딩 I/O 및 안전한 해제
* **이벤트 메모리 해제 금지:** 비동기 I/O 작업이 커널에 펜딩(`Pending`)된 상태일 때, 해당 이벤트(`OVERLAPPED` 파생 객체)를 메모리에서 해제해서는 절대 안 됩니다.
* **Graceful Shutdown:** 리스너나 세션을 종료할 때는 먼저 소켓을 닫아 커널이 펜딩된 작업을 실패 상태로 반환하도록 유도한 후, 이벤트가 반환되어 왔을 때 `_socket == INVALID_SOCKET`임을 검증하고 이벤트를 안전하게 소멸시켜야 합니다.

### Linux: `_epollRef`를 통한 참조 카운트 관리
* **문제:** `epoll_event.data.ptr`은 원시 포인터이므로, 멀티스레드 환경에서 통지 시점에 객체가 파괴되어 있으면 댕글링 포인터를 참조하게 됩니다.
* **해결 (`_epollRef`):** `NetObject`가 `NetCore::Register`로 Epoll에 등록될 때, 자신을 가리키는 `shared_ptr`인 `_epollRef`를 부여받습니다. 이 참조는 서비스나 세션이 `Disconnect` 될 때 안전하게 해제되어 객체의 생명주기를 보장합니다.

### 순환 참조 방지
* 어떠한 경우에도 이벤트 객체가 `NetObject`의 `shared_ptr`를 장기적으로 보유하게 만들고, `NetObject` 또한 그 이벤트를 `vector` 등에 영구적으로 들고 있으면 안 됩니다. 완료 큐(커널)와 이벤트 핸들러의 자연스러운 흐름에 의존해야 합니다.

---

## 5. 버퍼 및 메모리 관리 (Buffer & Memory)
* **RecvBuffer:** 데이터의 읽은 후 남은 데이터를 앞으로 당겨오는(Memory Move) 오버헤드를 줄이기 위해, ReadPos/WritePos로 파편화를 관리하는 링 버퍼.
* **SendBuffer & TLS:** 여러 스레드의 동시 `send` 락 경합을 막기 위해 TLS에 큰 메모리 블록(`SendBufferChunk`)을 사전 할당한 뒤 스레드 로컬에서 작은 단위로 쪼개어(`SendBuffer`) 반환합니다.

---

## 6. 스레드 동기화 및 락 프리 아키텍처 (Thread & Lock-Free)

`ServerCore`는 락(Lock) 경합으로 인한 성능 저하를 방지하기 위해 `JobQueue` 기반의 비동기 실행 모델을 채택했습니다.

### 1. Job과 JobQueue의 구조
* **`Job`**: 객체의 특정 메서드를 실행하기 위한 델리게이트(Callback)나 람다(Lambda)를 캡슐화한 객체입니다. 생명주기 동안 타겟 객체(`shared_ptr<T> owner`)의 참조 카운트를 유지하여, 실행 시점에 객체가 소멸되는 것을 방지합니다.
* **`JobQueue`**: 개별 객체(예: `GameRoom`, `Session`) 단위로 생성되며, 할당된 `Job`들을 락 프리 큐(`moodycamel::ConcurrentQueue`)에 적재합니다.

### 2. Lock-Free 실행 보장 원리 (`_isExecute` 플래그)
* `JobQueue`는 다수의 스레드가 동시에 `Push`를 하더라도, 내부의 `_isExecute` 원자적(atomic) 플래그를 통해 **오직 하나의 스레드만 큐의 소유권을 획득하여 작업을 순차적으로 실행(`Execute`)**하도록 제어합니다.
* 이를 통해 `JobQueue` 내에서 실행되는 로직은 절대로 여러 스레드에 의해 동시 실행되지 않음이 보장되며, 객체 내부의 데이터를 수정할 때 락(Mutex)을 걸 필요가 없어집니다.

### 3. 분산 처리와 기아(Starvation) 방지
* **Global Queue (`GGlobalJobQueue`)**: 특정 `JobQueue`에 너무 많은 작업이 몰려 한 스레드가 이를 독점 처리하게 되면, 다른 큐들이 기아 상태(Starvation)에 빠지거나 프레임 드랍이 발생할 수 있습니다.
* **제한 시간 실행**: 이를 막기 위해 `Execute(int32 excuteTime)` 함수는 할당된 시간(예: 10ms) 동안만 작업을 처리하고, 시간이 초과되면 처리하지 못한 남은 작업들을 가진 자신(`JobQueue` 자체)을 전역 큐(`GGlobalJobQueue`)에 반납한 뒤 스레드를 해제합니다. 이후 여유가 있는 다른 스레드가 전역 큐에서 해당 `JobQueue`를 꺼내어 남은 작업을 이어서 처리하게 됩니다.

### 4. 예약 작업 (`JobTimer`)
* **`JobTimer`**: 일정 시간 이후에 `Job`을 실행해야 할 때 사용됩니다(예: "5초 뒤에 몬스터 스폰"). 타이머 항목들은 실행 시간(`executeTick`)을 기준으로 우선순위 큐에 관리됩니다.
* 지정된 시간이 지나면 타이머가 `Job`을 해당 객체의 `JobQueue`로 `Push`하여, 스레드 안전하게 순차 실행 사이클에 편입시킵니다.

---

## 7. 크로스 플랫폼 컴파일 및 예외 처리 지침
* **소켓 초기화:** 소켓 생성 전 반드시 `SocketUtils::Init()`을 호출해야 합니다 (Windows의 `WSAStartup` 수행).
* **소켓 누수 방지:** 초기화 과정 실패 시 생성된 소켓은 `SocketUtils::Close()`로 즉각 반환해야 합니다.
* **조건부 컴파일 `#ifdef _WIN32` 유의점:** `Session` 등 양 플랫폼의 헤더가 결합된 코드에서 다른 플랫폼 빌드 시 구조체 멤버 누락으로 인한 빌드 에러가 발생하지 않도록 주의합니다.
* **인코딩:** GCC/WSL 호환성을 위해 소스 코드는 **BOM 없는 UTF-8** 형식이어야 합니다. BOM 포함 시 `#pragma once` 무시 등 재정의 에러가 발생합니다.