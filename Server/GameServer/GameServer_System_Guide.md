# GameServer 시스템 가이드 (System Guide)

이 문서는 서버의 객체 설계, 계층 구조 및 클라이언트 패킷에 따른 내부 처리 흐름을 종합적으로 설명합니다.

---

## 1. 객체 설계 및 계층 구조 (Object Architecture)

### **1.1 Class Hierarchy**
*   **GameObject (Base)**: 모든 게임 내 물체의 최상위 클래스. 고유 ID와 Transform(위치/회전) 관리.
*   **PlayerCharacter**: `GameObject`와 `AsyncActor`를 상속. 본인만의 `JobQueue`를 가지며 플레이어 상태(이동, 세션 등)를 처리.

### **1.2 필드 및 공간 관리 (Field)**
*   **공간 소유**: 특정 맵 인스턴스의 `dtNavMesh`를 소유하며, 해당 필드 내의 모든 플레이어를 관리.
*   **비동기 모델**: 필드별 `JobQueue`를 통해 `Enter`, `Leave`, `Move` 요청을 순차적으로 처리하여 데이터 정합성 유지.
*   **Lifecycle**: 플레이어가 0명이 되면 10초 후 자동 파괴 (`_destroyToken` 기반).

---

## 2. 패킷 처리 워크플로우 (Packet Workflow)

### **2.1 게임 입장 (Initial Entry)**
1.  **[CS] `CS_ENTER_GAME`**: `GameManager::ProcessEnterGame` 호출.
2.  **예약**: 플레이어 객체 생성 및 목적지 맵/좌표 설정 (`SetLoadingInfo`).
3.  **[SC] `SC_START_FIELD_LOADING`**: 클라이언트에게 로딩 시작 전송.

### **2.2 로딩 완료 및 스폰 (Loading & Spawning)**
1.  **[CS] `CS_FIELD_LOADING_COMPLETE`**: `Field::EnterPlayer` 호출.
2.  **확정**: `GetPendingSpawnPos()`를 실제 좌표로 적용.
3.  **동기화**: `SC_ENTER_FIELD` (본인), `SC_SPAWN_PLAYER` (주변 브로드캐스트) 전송.

### **2.3 이동 시스템 (Movement)**
1.  **[CS] `CS_REQUEST_MOVE`**: `Field::PlayerRequestMove` 호출.
2.  **최적화 (Raycast)**: 직선상에 장애물이 없으면 즉시 경로 생성 (4~6us).
3.  **경로 계산 (Full Path)**: 장애물 발견 시 NavMesh 기반 A* 연산 (10~11us).
4.  **[SC] `SC_MOVE_PATH`**: 계산된 Waypoints를 주변에 브로드캐스트.

### **2.4 포탈 및 필드 이동 (Portal & Transition)**
1.  **[CS] `CS_USE_PORTAL`**: `Field::RequestUsePortal` 호출 (거리 검증: 500cm).
2.  **예약 (Transition)**: `GameManager::ProcessMoveField`에서 새 목적지 좌표 계산 및 로딩 패킷 전송.
3.  **재진입**: 로딩 완료 패킷 수신 시 위의 **'2.2 단계'**부터 다시 수행.

---

## 3. 핵심 운영 원칙
*   **Thread Safety**: 모든 상태 변경은 `JobQueue`(`DoAsync`)를 통해 수행하여 Lock 경쟁 최소화.
*   **Coordinate Sync**: Detour(m)와 Engine(cm) 간의 단위 변환(X,Z,Y ↔ X,Y,Z)을 엄격히 준수.
*   **Memory Safety**: `shared_ptr`과 `weak_ptr`을 활용하여 객체 생명주기 및 순환 참조 관리.
