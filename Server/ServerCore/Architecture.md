# ServerCore Architecture & Implementation Guide

이 문서는 게임 서버의 핵심 네트워크 엔진인 `ServerCore` 프로젝트의 전반적인 구조와 구현 방식, 그리고 각 폴더와 파일이 수행하는 역할을 상세하게 정리한 기술 문서입니다.

## 1. 프로젝트 전체 구성 및 핵심 원리

`ServerCore`는 대규모 동시 접속 환경에서 병목 현상을 최소화하고 고성능 처리를 하기 위해 설계된 비동기 네트워크/스레딩 라이브러리입니다.

### 핵심 설계 철학
1.  **비동기 I/O (Asynchronous I/O)**: Windows의 IOCP(I/O Completion Port)를 사용하여 소켓 통신 과정에서 스레드가 대기(Block)하지 않도록 구현되었습니다.
2.  **Lock-Free 지향 및 JobQueue 모델**: 멀티스레드 환경에서 자원 접근 시 발생하는 락(Lock) 경합(Contention)을 줄이기 위해, 객체별 순차 실행을 보장하는 `JobQueue`를 도입했습니다.
3.  **메모리 및 버퍼 최적화**: 잦은 동적 할당/해제로 인한 성능 저하를 막기 위해, 스레드 로컬 저장소(TLS)와 연속적인 링 버퍼(`RecvBuffer`), Chunk 기반 풀링 등을 적극 활용합니다.

---

## 2. 폴더별 상세 구현 및 파일의 역할

프로젝트는 역할에 따라 크게 5개의 폴더로 나뉘어 있습니다.

### Platform/Windows (OS 종속적 네트워크 커널)
운영체제(Windows)가 제공하는 비동기 I/O 기능을 객체지향적으로 래핑한 폴더입니다. (추후 리눅스 이식 시 `Platform/Linux` 등으로 분기할 수 있는 구조)

*   **`IocpEvent.h / .cpp`**
    *   Windows의 `OVERLAPPED` 구조체를 상속받은 이벤트 클래스입니다.
    *   수행된 비동기 작업의 종류(Accept, Connect, Disconnect, Recv, Send)를 식별하기 위한 `EventType` 태그와, 이벤트를 소유한 객체(`IocpObjectRef`)의 정보를 담고 있습니다.
*   **`IocpCore.h / .cpp`**
    *   IOCP 커널 핸들을 캡슐화한 클래스입니다.
    *   `Register()`: 소켓이나 핸들을 IOCP 큐에 등록합니다 (`CreateIoCompletionPort` 활용).
    *   `Dispatch()`: 스레드 풀의 워커 스레드들이 호출하며, 완료된 I/O 작업을 꺼내어(`GetQueuedCompletionStatus`) 해당 객체의 콜백 함수로 넘겨줍니다.

### Network (네트워크 연결 및 세션 관리)
실질적인 소켓 통신 흐름을 제어하는 계층입니다. `IocpObject`를 상속받아 IOCP 커널 위에서 동작합니다.

*   **`Session.h / .cpp`**
    *   클라이언트와의 개별 연결을 나타내는 가장 핵심적인 파일입니다.
    *   **Recv 처리**: `RegisterRecv()`로 OS에 비동기 수신을 요청하고, 완료되면 `ProcessRecv()`에서 `RecvBuffer`에 데이터를 쓰고, 완전한 패킷으로 조립하여(`PacketSession`의 역할) 상위 로직에 넘깁니다.
    *   **Send 처리**: `Send()` 호출 시 즉시 보내지 않고 내부 큐에 쌓은 뒤, `RegisterSend()`를 통해 모인 버퍼들을 한 번의 `WSASend`로 모아서 보내는 최적화(Gathering)가 구현되어 있습니다.
*   **`Listener.h / .cpp`**
    *   서버에서 클라이언트의 접속 요청을 비동기로 대기(`AcceptEx` 활용)하는 문지기 객체입니다.
    *   접속이 수락되면 `Service`에 등록된 팩토리 함수를 이용해 새로운 `Session`을 생성합니다.
*   **`Service.h / .cpp`**
    *   `ServerService`(리스너 포함)와 `ClientService`(외부 서버 접속용)로 나뉘며, IP/포트 정보와 생성된 세션들의 목록(`set<SessionRef>`)을 통합 관리합니다.
*   **`NetAddress.h / .cpp`** & **`SocketUtils.h / .cpp`**
    *   IPv4/IPv6 주소를 감싸는 래퍼 클래스와, Linger, ReuseAddress, TCP_NODELAY 등 소켓 옵션 및 헬퍼 함수들을 모아둔 유틸리티입니다.

### Buffer (메모리 및 스트림 관리)
네트워크를 타고 들어오고 나가는 바이트 스트림(Byte Stream)을 안전하게 처리합니다.

*   **`RecvBuffer.h / .cpp`**
    *   TCP의 파편화(Fragmentation) 문제를 해결하기 위해 사용하는 링 버퍼 형식의 클래스입니다.
    *   쓰기 포인터(`WritePos`)와 읽기 포인터(`ReadPos`)를 가지며, 패킷이 잘려서 왔을 때는 읽기를 멈추고 공간이 부족해지면 남은 유효 데이터를 버퍼의 맨 앞으로 밀착시키는 `Clean()` 작업을 수행합니다.
*   **`SendBuffer.h / .cpp`**
    *   응답 패킷을 담을 메모리 버퍼입니다. 메모리 단편화를 줄이고 재사용성을 높이기 위한 구조로 설계되어 있습니다.

### Thread (멀티스레드 및 동기화)
이 서버 프레임워크의 가장 강력한 특징인 락-프리(Lock-Free) 지향 작업 처리 모델입니다.

*   **`JobQueue.h / .cpp` & `Job.h / .cpp`**
    *   객체의 상태를 변경하는 작업들을 람다(Lambda)나 함수 포인터 형태(`Job`)로 큐에 넣고 순차적으로 실행하게 만드는 클래스입니다.
    *   여러 스레드가 동시에 한 객체(예: 게임 방)에 접근하려고 할 때 락을 거는 대신 큐에 작업을 쌓고(Push), 큐가 비어있을 때 처음 접근한 스레드가 큐를 통째로 비우면서(Pop & Execute) 실행을 전담하여 데드락(Deadlock)의 위험을 원천적으로 차단합니다.
*   **`JobTimer.h / .cpp`**
    *   지정된 시간이 지난 후 특정 `Job`이 실행되도록 예약하는 타이머 객체입니다. (예: 5초 뒤 몬스터 스폰)
*   **`concurrentqueue.h`**
    *   멀티스레드 환경에서 Lock 없이 빠르게 동작하는 오픈소스(Moodycamel) 큐입니다. `JobQueue` 등 코어 내부의 고성능 자료구조로 활용됩니다.

### Main (공통 매크로 및 타입 정의)
프로젝트 전반에서 쓰이는 설정, 매크로, 스마트 포인터 타입 등이 정의되어 있습니다.

*   **`CoreMacro.h`** & **`Types.h`**
    *   크래시 유도 매크로(`CRASH`), 메모리 락 매크로(`USE_LOCK`), 고정 크기 정수형(`int32`, `uint64`), 그리고 엔진 전반에서 쓰이는 스마트 포인터 타입(`SessionRef`, `SendBufferRef`) 등을 정의합니다.
*   **`CoreTLS.h / .cpp`**
    *   스레드 로컬 저장소(Thread Local Storage) 변수들을 선언합니다. 스레드 고유의 ID나 작업 중인 글로벌 큐(Send 큐, Job 큐 등)에 락 없이 접근하기 위해 사용됩니다.
*   **`Singleton.h`**
    *   매니저 클래스 등에서 널리 쓰이는 싱글톤 패턴 템플릿입니다.

---

## 3. 핵심 모듈 간 상호작용 (Implementation Flow)

위에서 설명한 모듈들이 결합되어 실제로 동작하는 흐름은 다음과 같습니다.

1.  **서버 부팅 단계**: 메인 프로세스에서 스레드 풀을 생성하고, `IocpCore`와 묶인 `ServerService`를 `Start()`하여 `Listener`가 접속 대기 상태에 들어갑니다. 스레드 풀의 워커 스레드들은 `IocpCore->Dispatch()`를 호출하며 OS의 통지를 무한정 기다립니다.
2.  **클라이언트 접속**: 클라이언트가 붙으면 커널에서 이벤트를 발생시키고, 대기 중이던 워커 스레드가 깨어나 `Session`을 생성한 뒤 `Session->RegisterRecv()`로 첫 데이터 수신을 예약합니다.
3.  **데이터 처리 및 로직 실행**: 클라이언트가 보낸 데이터가 `RecvBuffer`에 쌓이면 패킷 크기에 맞게 조립됩니다. 조립이 완료된 패킷은 게임 로직(예: 몬스터 공격)을 호출해야 하는데, 이때 직접 실행하며 락을 걸지 않고 **`JobQueue::Push()`를 통해 객체의 큐에 작업을 밀어 넣습니다.**
4.  **응답 송신 최적화**: 게임 로직의 결과물(응답 패킷)은 다시 클라이언트로 보내져야 합니다. 이때 각 세션은 `Session::Send()`를 호출하는데, 바로 OS의 송신 함수를 부르지 않고 내부 `SendQueue`에 쌓아둡니다. 특정 시점(보통 큐에 처음 쌓였을 때나 워커 스레드가 다른 일을 끝냈을 때)에 한 번의 `WSASend`로 모아서 전송하여 네트워크 부하를 극도로 낮춥니다.

이러한 구조는 C++의 고성능 멀티스레드 환경에서 **동기화 병목 현상을 타파**하고 커널 모드/유저 모드 전환 비용을 최소화하기 위한 가장 이상적인 게임 서버 프레임워크 설계입니다.