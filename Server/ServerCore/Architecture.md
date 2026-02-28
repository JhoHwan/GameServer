# ServerCore Architecture & Implementation Guide

이 문서는 게임 서버의 핵심 네트워크 엔진인 `ServerCore` 프로젝트의 전반적인 구조와 구현 방식, 그리고 각 폴더와 파일이 수행하는 역할을 상세하게 정리한 기술 문서입니다. 현재 프로젝트는 Windows(IOCP)와 Linux(Epoll)를 모두 지원하기 위한 멀티 플랫폼 추상화가 적용되어 있습니다.

## 1. 프로젝트 전체 구성 및 핵심 원리

`ServerCore`는 대규모 동시 접속 환경에서 병목 현상을 최소화하고 고성능 처리를 하기 위해 설계된 비동기 네트워크/스레딩 라이브러리입니다.

### 핵심 설계 철학
1.  **멀티 플랫폼 비동기 I/O**: Windows의 IOCP와 Linux의 Epoll을 공통 인터페이스(`INetCore`)로 추상화하여, 동일한 비즈니스 로직이 OS에 관계없이 동작하도록 설계되었습니다.
2.  **Lock-Free 지향 및 JobQueue 모델**: 멀티스레드 환경에서 자원 접근 시 발생하는 락(Lock) 경합(Contention)을 줄이기 위해, 객체별 순차 실행을 보장하는 `JobQueue`를 도입했습니다.
3.  **메모리 및 버퍼 최적화**: 잦은 동적 할당/해제로 인한 성능 저하를 막기 위해, 스레드 로컬 저장소(TLS)와 연속적인 링 버퍼(`RecvBuffer`), Chunk 기반 풀링 등을 적극 활용합니다.

---

## 2. 폴더별 상세 구현 및 파일의 역할

프로젝트는 역할에 따라 크게 6개의 폴더로 나뉘어 있습니다.

### Network (공통 네트워크 인터페이스 및 로직)
플랫폼에 관계없이 공통으로 사용되는 네트워크 추상화 레이어입니다.

*   **`INetCore.h` & `INetObject.h`**
    *   네트워크 엔진(`NetCore`)과 통신 객체(`NetObject`)의 최상위 인터페이스입니다.
    *   플랫폼별 구현체(IOCP, Epoll)는 이 인터페이스를 상속받아 구현됩니다.
*   **`NetEvent.h / .cpp`**
    *   플랫폼별 이벤트 객체(IocpEvent, EpollEvent)를 `using NetEvent = ...`를 통해 하나로 추상화합니다.
    *   `ConnectEvent`, `RecvEvent`, `SendEvent` 등 세부 이벤트 클래스들이 정의되어 있습니다.
*   **`Session.h / .cpp` & `PacketSession.h / .cpp`**
    *   `INetObject`를 상속받아 실제 통신 로직을 수행하는 클래스입니다.
    *   데이터 수신 시 패킷 단위로 조립하여 상위 로직에 전달하는 역할을 수행합니다.
*   **`Listener.h / .cpp`**
    *   서버의 접속 대기 로직을 담당하며, 새로운 연결 발생 시 세션을 생성합니다.

### Platform/Windows (Windows 전용 구현)
Windows 환경에서 IOCP를 이용해 네트워크 I/O를 처리하는 구현체들입니다.

*   **`IocpCore.h / .cpp`**: `INetCore`의 Windows 구현체로, IOCP 핸들을 관리하고 이벤트를 Dispatch합니다.
*   **`IocpEvent.h`**: Windows의 `OVERLAPPED` 구조체를 상속받아 IOCP 완료 통지를 처리하기 위한 전용 이벤트 클래스입니다.

### Platform/Linux (Linux 전용 구현)
Linux 환경에서 Epoll을 이용해 네트워크 I/O를 처리하는 구현체들입니다.

*   **`EpollEvent.h`**: Linux 환경에서 Epoll 이벤트를 처리하기 위한 전용 이벤트 클래스입니다.

### Buffer (메모리 및 스트림 관리)
네트워크를 타고 들어오고 나가는 바이트 스트림(Byte Stream)을 안전하게 처리합니다.

*   **`RecvBuffer.h / .cpp`**: TCP 파편화 문제를 해결하는 링 버퍼입니다.
*   **`SendBuffer.h / .cpp`**: 송신 데이터를 담는 메모리 버퍼로, 단편화를 최소화하도록 설계되었습니다.

### Thread (멀티스레드 및 동기화)
락-프리 지향 작업 처리 모델과 스레드 관리 로직이 포함되어 있습니다.

*   **`JobQueue.h / .cpp`**: 작업을 큐에 쌓아 순차적으로 실행함으로써 락 경합을 방지하는 핵심 모듈입니다.
*   **`JobTimer.h / .cpp`**: 예약된 시간에 작업을 실행하는 타이머입니다.

### Main (공통 설정 및 유틸리티)
*   **`Types.h`**: 고정 크기 자료형 및 스마트 포인터(`Ref`) 타입들을 정의합니다.
*   **`CorePch.h` & `pch.h`**: 미리 컴파일된 헤더로, 자주 사용되는 표준 라이브러리와 공통 헤더를 포함합니다.

---

## 3. 핵심 모듈 간 상호작용 (Implementation Flow)

추상화된 인터페이스를 통한 동작 흐름은 다음과 같습니다.

1.  **서버 초기화**: `ServerService` 생성 시 플랫폼에 맞는 `NetCore`(Windows의 경우 `IocpCore`)가 주입됩니다.
2.  **접속 대기**: `Listener`가 `NetCore`에 등록되어 비동기 Accept를 수행합니다.
3.  **이벤트 루프**: 워커 스레드들은 `NetCore->Dispatch()`를 호출하며 OS로부터 I/O 완료 또는 준비 통지를 기다립니다.
4.  **콜백 처리**: 이벤트 발생 시 `NetCore`는 해당 `NetObject`(Session 또는 Listener)의 `Dispatch()`를 호출하여 비즈니스 로직을 실행합니다.
5.  **로직 실행 및 송신**: 수신된 패킷은 `JobQueue`를 통해 안전하게 처리되며, 응답은 `Session->Send()`를 통해 모아서 전송(Gathering)됩니다.

이 구조는 플랫폼별 상세 구현을 은닉하면서도, 상위 계층에서는 단일화된 인터페이스를 통해 높은 생산성과 성능을 동시에 확보할 수 있도록 설계되었습니다.