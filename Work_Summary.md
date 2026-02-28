# 오늘 작업 요약 (GameServer Network Core)

## 1. 크로스 플랫폼 네트워크 엔진 추상화 (Windows & Linux)
* **공통 인터페이스 도입**: 기존 윈도우 종속적이었던 `IocpCore`, `IocpObject`, `IocpEvent` 구조를 각각 `NetCore`, `NetObject`, `NetEvent`라는 플랫폼 독립적인 인터페이스로 추상화.
* **플랫폼별 구현 분리**: Windows(IOCP)와 추후 구현될 Linux(Epoll)의 로직을 하위 클래스로 분리. `using NetEvent = IocpEvent;` 와 같은 조건부 타입 치환(Type Aliasing)을 통해 비즈니스 로직(상위 레이어)이 OS에 종속되지 않고 동일한 코드로 동작할 수 있도록 아키텍처 개선.
* **Pimpl 패턴 적용**: `Listener` 클래스에 Pimpl 패턴을 적용하여 헤더 파일에서 플랫폼 종속성(`<winsock2.h>` 등)을 제거하고, 크로스 플랫폼 빌드 시 발생하는 Incomplete Type 에러 방지 처리 완료.

## 2. 크리티컬 버그 및 리소스 누수 수정
* **메모리 오염 및 서버 크래시 방지**: `Listener_Win` 소멸자에서 `AcceptEvent`를 강제 `delete` 하던 로직 제거. 커널의 비동기 I/O 작업(Pending I/O)이 취소되어 워커 스레드로 반환될 때 소켓 상태(`INVALID_SOCKET`)를 확인하여 안전하게 메모리를 해제하도록 수정.
* **순환 참조 제거**: `ListenerImpl` 내부의 `vector<AcceptEvent*>`를 제거하여, `AcceptEvent`와 `Listener` 간의 메모리 누수를 유발하는 순환 참조 고리 절단.
* **소켓 누수 방지**: `Listener::StartAccept` 초기화 도중 실패 시, 생성된 소켓을 `SocketUtils::Close()`로 닫고 반환하도록 RAII 기반(또는 방어적) 흐름으로 개선.

## 3. Dummy 테스트 프로젝트 구축
* 순수 네트워크 코어 기능(`ServerCore`)만을 검증하기 위해 `DummyServer`와 `DummyClient` 프로젝트를 CMake로 구성.
* 100명 이상의 대규모 접속 상황을 모사하여 `ClientService`와 `ServerService`의 부하 처리 및 `Accept` 이벤트 풀링 정상 작동 확인.
* 워커 스레드의 무한 루프를 `atomic<bool> GIsRunning` 플래그로 제어하여 `quit` 명령어 입력 시 **Graceful Shutdown (안전한 종료)**이 이뤄지도록 구현.

## 4. 네트워크 연결 에러 (에러 코드 87) 해결
* `DummyClient`에서 발생한 87(ERROR_INVALID_PARAMETER) 에러의 원인이 `WSAStartup` 누락임을 파악.
* `ClientService::Start()` 최상단에 `SocketUtils::Init()`을 추가하여 WinSock 초기화 문제를 해결하고 정상적인 소켓 생성 및 IOCP 등록 보장.

## 5. 글로벌 메모리 (GEMINI.md) 업데이트
* 위 작업들을 통해 얻은 설계 원칙, 라이프사이클 관리법, 문제 해결 내역을 `GEMINI.md`에 영구 기록하여 향후 개발 방향성 및 컨텍스트 유지.

## 6. 내일 할 일 (Next Steps)
* **Session 클래스 Pimpl 패턴 적용**: `Listener`와 동일하게 `Session` 객체도 Pimpl 패턴을 적용하여 플랫폼 종속적인 비동기 송수신(Recv/Send) 로직을 `Session_Win`과 `Session_Linux`로 완벽히 분리.
* **Linux (Epoll) 구현체 작성**: 마련된 크로스 플랫폼 추상화(`NetCore`, `NetEvent` 등)를 바탕으로 `Platform/Linux` 폴더 내에 `EpollCore`, `Listener_Linux`, `Session_Linux` 등의 실제 동작 코드를 구현하여 리눅스 환경 빌드 및 구동 테스트 준비.