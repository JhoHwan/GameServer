# ServerCore Architecture & Implementation Guide

이 문서는 게임 서버의 핵심 네트워크 엔진인 `ServerCore` 프로젝트의 전반적인 구조와 구현 방식, 그리고 각 폴더와 파일이 수행하는 역할을 상세하게 정리한 기술 문서입니다. 현재 프로젝트는 Windows(IOCP)와 Linux(Epoll)를 모두 지원하기 위한 멀티 플랫폼 추상화가 적용되어 있습니다.

## 1. 프로젝트 전체 구성 및 핵심 원리

`ServerCore`는 대규모 동시 접속 환경에서 병목 현상을 최소화하고 고성능 처리를 하기 위해 설계된 비동기 네트워크/스레딩 라이브러리입니다.

### 핵심 설계 철학
1. **멀티 플랫폼 비동기 I/O (인터페이스 분리)**: Windows의 IOCP와 Linux의 Epoll을 공통 인터페이스(`NetCore`)로 추상화하였습니다. 비즈니스 로직(상위 레이어)은 OS에 종속되지 않고 `NetCore`, `NetObject`, `NetEvent`와 같은 공통 추상화 레이어를 통해 통신합니다.
2. **Lock-Free 지향 및 JobQueue 모델**: 멀티스레드 환경에서 자원 접근 시 발생하는 락(Lock) 경합(Contention)을 줄이기 위해, 객체별 순차 실행을 보장하는 `JobQueue`를 도입했습니다.
3. **Pimpl (Pointer to Implementation) 패턴**: 플랫폼 종속적인 헤더(예: `<winsock2.h>`)가 상위 로직에 노출되는 것을 방지하기 위해 `Listener` 클래스 등에 Pimpl 패턴을 적용했습니다. 이를 통해 플랫폼 독립적인 인터페이스와 플랫폼 종속적인 비동기 구현체(`Listener_Win`, 추후 `Listener_Linux`)를 완벽히 분리합니다.
4. **메모리 및 버퍼 최적화**: 잦은 동적 할당/해제로 인한 성능 저하를 막기 위해, 스레드 로컬 저장소(TLS)와 연속적인 링 버퍼(`RecvBuffer`), Chunk 기반 풀링(`SendBuffer`) 등을 적극 활용합니다.

---

## 2. 폴더별 상세 구현 및 파일의 역할

프로젝트는 역할에 따라 여러 폴더로 나뉘어 있습니다.

### Network (공통 네트워크 인터페이스 및 로직)
플랫폼에 관계없이 공통으로 사용되는 네트워크 추상화 레이어입니다.

* **`NetCore.h / .cpp` & `NetObject.h / .cpp`**
  * 네트워크 엔진(`NetCore`)과 통신 객체(`NetObject`)의 최상위 인터페이스입니다.
  * 플랫폼별 구현체(IOCP, Epoll)는 내부적으로 이 구조에 맞춰 동작하게 됩니다.
* **`NetEvent.h / .cpp`**
  * 플랫폼별 이벤트 객체(IocpEvent, EpollEvent)를 조건부 타입 치환(`using NetEvent = IocpEvent;` 등)을 통해 하나로 추상화합니다.
  * `ConnectEvent`, `RecvEvent`, `SendEvent`, `AcceptEvent` 등 세부 이벤트 클래스들이 정의되어 있습니다.
* **`Session.h / .cpp`**
  * `NetObject`를 상속받아 실제 통신 로직을 수행하는 클래스입니다.
  * **[진행 예정]**: `Listener`와 동일하게 Pimpl 패턴을 적용하여 `Session_Win`과 `Session_Linux`로 플랫폼 종속 로직을 분리할 예정입니다.
* **`Listener.h / .cpp`**
  * 서버의 접속 대기 로직을 담당합니다. Pimpl 패턴을 사용해 `ListenerImpl`에 실제 동작을 위임합니다.
* **`SocketUtils.h / .cpp`**
  * 소켓 생성, 바인딩, 초기화 등 소켓 관련 유틸리티를 제공합니다.
  * 소켓 생성 전에 반드시 `SocketUtils::Init()`(내부적으로 `WSAStartup` 등 호출)이 호출되어야 오류(예: 87 `ERROR_INVALID_PARAMETER`)를 방지할 수 있습니다.

### Platform (플랫폼 전용 구현체)
운영체제별 비동기 I/O 모델을 처리하는 폴더입니다.

* **Windows (`Platform/Windows`)**
  * **`IocpCore.h / .cpp`**: `NetCore`의 인터페이스를 충족하는 Windows IOCP 핸들 관리 및 Dispatch 구현체.
  * **`IocpEvent.h / .cpp`**: Windows의 `OVERLAPPED` 구조체를 상속받아 완료 통지를 처리하기 위한 이벤트.
  * **`Listener_Win.h / .cpp`**: `ListenerImpl`의 Windows IOCP 버전.
* **Linux (`Platform/Linux`)**
  * **`EpollEvent.h / .cpp`**: Linux 환경에서 Epoll 이벤트를 처리하기 위한 이벤트 구조체.
  * **[진행 예정]**: `EpollCore`, `Listener_Linux`, `Session_Linux` 등 본격적인 Linux 구동체 개발 예정.

### Buffer (메모리 및 스트림 관리)
네트워크를 타고 들어오고 나가는 바이트 스트림(Byte Stream)을 안전하게 처리합니다.

* **`RecvBuffer.h / .cpp`**: TCP 파편화(Fragmentation) 문제를 해결하는 링 버퍼.
* **`SendBuffer.h / .cpp`**: 송신 데이터를 담는 메모리 버퍼. 단편화를 최소화하도록 Chunk 형태로 설계됨.

### Thread (멀티스레드 및 동기화)
락-프리 지향 작업 처리 모델과 스레드 관리 로직이 포함되어 있습니다.

* **`JobQueue.h / .cpp`**: 작업을 큐에 쌓아 순차적으로 실행함으로써 락 경합을 방지하는 핵심 모듈.
* **`JobTimer.h / .cpp`**: 예약된 시간에 작업을 실행하는 타이머.
* **`concurrentqueue.h`**: 스레드 안전한 락프리 큐 라이브러리 (Third-party).

### Main (공통 설정 및 유틸리티)
* **`Types.h`**: 고정 크기 자료형 및 스마트 포인터(`Ref`) 타입들을 정의합니다.
* **`CorePch.h` & `pch.h`**: 미리 컴파일된 헤더로, 자주 사용되는 표준 라이브러리와 공통 헤더를 포함합니다.
* **`CoreTLS.h / .cpp`**: 스레드 로컬 스토리지(TLS) 전역 변수(예: SendBufferChunk) 관리.

---

## 3. 안정성 및 메모리 관리 지침 (Critical Guidelines)

### 비동기 I/O (Pending I/O) 및 라이프사이클 관리
1. **이벤트 메모리 해제 금지**: IOCP(Windows) 등에서 비동기 작업이 커널에 펜딩(`Pending`)된 상태일 때 해당 이벤트(`OVERLAPPED` 파생 객체 등)를 절대 강제로 `delete`해서는 안 됩니다. 
2. **Graceful Shutdown**: 
   * 리스너나 세션을 종료할 때는 먼저 소켓을 닫습니다(`CloseSocket()`).
   * 소켓이 닫히면 커널은 펜딩된 작업을 취소하고 실패 상태로 워커 스레드에 반환합니다.
   * 워커 스레드는 이벤트를 받아 소켓이 `INVALID_SOCKET` 상태인지 확인한 후, 그제서야 안전하게 이벤트 객체(`delete acceptEvent`)를 메모리 해제해야 합니다.
3. **순환 참조 고리 절단**: 이벤트가 `NetObject`(예: `Listener`)의 `shared_ptr`를 들고 있는 상태에서, 객체가 다시 해당 이벤트를 컨테이너(예: `vector`)에 저장하면 순환 참조로 인해 메모리 누수가 발생합니다. 이벤트의 생명주기는 오직 I/O 완료 큐(커널)와 워커 스레드에 맡겨야 합니다.

### 리소스 초기화 및 예외 처리 방어
* **소켓 초기화 누수 방지**: `Listener::StartAccept` 등의 과정 중 `NetCore->Register()`와 같이 중간 단계에서 실패할 경우, 생성해둔 소켓은 `SocketUtils::Close()`를 통해 즉시 해제한 후 false를 반환해야 합니다.

### 크로스 플랫폼 빌드 주의사항
* **Incomplete Type 대응**: `unique_ptr`를 전방 선언(Forward Declaration)된 Pimpl 클래스와 함께 사용할 경우, 포인터가 파괴되는 소멸자(Destructor) 위치에서 구현 클래스의 전체 정의(Full Definition)가 반드시 노출되어야 합니다.
* 플랫폼별 매크로(`#ifdef _WIN32`) 사용 시 다른 플랫폼(Linux) 빌드 환경에서 헤더나 구조체가 누락되어 Incomplete Type 에러가 발생하지 않도록, 필요한 빈 선언(Dummy class) 등을 철저히 포함해야 합니다.

---

## 4. 네트워크 엔진 테스트 (Dummy Project)

* `ServerCore` 엔진의 성능 및 안정성을 순수하게 검증하기 위해 `Server/DummyServer` 및 `Server/DummyClient` 프로젝트를 활용합니다.
* **부하 테스트 전략**: 다수의 세션 연결/해제를 빠르게 반복하며 `Accept` 이벤트 풀링과 워커 스레드의 안정성을 스트레스 테스트합니다.
* **안전한 종료 (Graceful Shutdown) 제어**: `atomic<bool> GIsRunning` 전역 플래그를 사용하여, 메인 스레드에서 종료 명령(`quit`)이 내려올 때 워커 스레드들이 진행 중인 `NetCore->Dispatch()`를 무사히 탈출할 수 있도록 유도합니다.

이 구조와 지침을 기반으로 플랫폼별 비동기 I/O 기능이 투명하고 일관되게 서비스 코드로 전달되며, 높은 개발 생산성을 보장합니다.