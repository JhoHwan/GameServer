# GameServer 폴더의 모든 소스 코드 찾기
file(GLOB_RECURSE GAME_SOURCES "*.cpp" "*.h" "*.cc")

# --- [Protobuf 및 패킷 자동 생성 설정] ---

# 1. 입력 파일(Proto)과 출력 폴더 지정
set(PROTO_FILE "${CMAKE_CURRENT_SOURCE_DIR}/Proto/Protocol.proto") # 자동화할 대상 파일
set(PROTO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Proto")
set(PACKET_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Packet")

# 2. 실행할 도구들 경로 지정
set(PROTOC_EXE "${CMAKE_CURRENT_SOURCE_DIR}/../../ThirdParty/Protobuf/protoc.exe")
set(PACKET_GEN_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/../PacketGenerator/PacketGenerator.py")
set(CLIENT_GEN_BAT "${CMAKE_CURRENT_SOURCE_DIR}/../PacketGenerator/ClientPacketGen.bat")

# 3. 사용자 지정 빌드 명령 (VS Custom Build Tool과 완벽 동일)
add_custom_command(
    # 이 명령어를 통해 만들어지는 결과물 (이게 있어야 CMake가 의존성을 파악해)
    OUTPUT "${PROTO_DIR}/Protocol.pb.cc" "${PROTO_DIR}/Protocol.pb.h"
    
    # (1) Protoc 컴파일러 실행
    COMMAND ${PROTOC_EXE} -I="${PROTO_DIR}" --cpp_out="${PROTO_DIR}" "${PROTO_FILE}"
    
    # (2) 파이썬 패킷 제너레이터 실행
    COMMAND python "${PACKET_GEN_SCRIPT}" --path "${PROTO_FILE}" --output ServerPacketHandler --out_path "${PACKET_DIR}" --process S
    
    # (3) 클라이언트 패킷 복사 배치파일 실행
    COMMAND "${CLIENT_GEN_BAT}"
    
    # 이 파일(Protocol.proto)이 수정되었을 때만 위 명령어들을 다시 실행함!
    DEPENDS "${PROTO_FILE}"
    
    # 빌드 창에 띄울 메시지
    COMMENT "Generating Protocol & PacketHandler from Protocol.proto..."
)

# GameServer라는 이름의 실행 파일(.exe) 만들기
add_executable(GameServer ${GAME_SOURCES})

# GameServer 전용 pch.h 설정
target_precompile_headers(GameServer PRIVATE pch.h)

# ServerCore 조립하기! 
target_link_libraries(GameServer PRIVATE ServerCore)

# 서드파티(Protobuf 등) 헤더 경로 추가
target_include_directories(GameServer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Proto   # <--- 이거 추가!
    ${CMAKE_CURRENT_SOURCE_DIR}/../../ThirdParty/Protobuf/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/Detour/Include
)

# Protobuf 정적 라이브러리 파일(.lib) 연결
if(WIN32)
    target_link_libraries(GameServer PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../ThirdParty/Protobuf/Lib/libprotobufd.lib
    )
endif()