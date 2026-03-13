# GameServer 폴더의 모든 소스 코드 찾기
file(GLOB_RECURSE GAME_SOURCES "*.cpp" "*.h" "*.cc")

# --- [Protobuf 및 패킷 자동 생성 설정] ---

# 1. 입력 파일(Proto)과 출력 폴더 지정
set(PROTO_FILE "${CMAKE_CURRENT_SOURCE_DIR}/Proto/Protocol.proto") # 자동화할 대상 파일
set(STRUCT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/Proto/Struct.proto") # Struct 파일도 함께 관리
set(PROTO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Proto")
set(PACKET_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Packet")

# 2. 실행할 도구들 경로 지정
find_package(Python3 REQUIRED)
set(PYTHON_EXE Python3::Interpreter)

if (WIN32)
    set(PROTOC_EXE "${CMAKE_CURRENT_SOURCE_DIR}/../../ThirdParty/Protobuf/protoc.exe")
    set(PACKET_GEN_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/../PacketGenerator/PacketGenerator.py")
    set(CLIENT_GEN_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/../PacketGenerator/PacketGen.bat")
else()
    set(PROTOC_EXE "${CMAKE_CURRENT_SOURCE_DIR}/../../ThirdParty/Protobuf/protoc")
    set(PACKET_GEN_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/../PacketGenerator/PacketGenerator.py")
    set(CLIENT_GEN_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/../PacketGenerator/PacketGen.sh")
endif ()

# 3. 사용자 지정 빌드 명령 (윈도우/리눅스 공통)
set(PROTO_OUTPUTS 
    "${PROTO_DIR}/Protocol.pb.cc" "${PROTO_DIR}/Protocol.pb.h" 
    "${PROTO_DIR}/Struct.pb.cc" "${PROTO_DIR}/Struct.pb.h"
)

# (1) Protoc 컴파일러 실행 (공통)
add_custom_command(
    OUTPUT ${PROTO_OUTPUTS}
    COMMAND ${PROTOC_EXE} -I="${PROTO_DIR}" --cpp_out="${PROTO_DIR}" "${PROTO_FILE}" "${STRUCT_FILE}"
    DEPENDS "${PROTO_FILE}" "${STRUCT_FILE}"
    COMMENT "Generating Protocol & Struct..."
)

# (2) 파이썬 패킷 제너레이터 실행 (공통)
add_custom_command(
    OUTPUT "${PACKET_DIR}/ServerPacketHandler.h"
    COMMAND ${PYTHON_EXE} "${PACKET_GEN_SCRIPT}" --path "${PROTO_FILE}" --output ServerPacketHandler --out_path "${PACKET_DIR}" --process S
    DEPENDS "${PROTO_FILE}" "${STRUCT_FILE}" "${PACKET_GEN_SCRIPT}"
    COMMENT "Generating ServerPacketHandler..."
)

add_executable(GameServer 
    ${GAME_SOURCES}
    ${PROTO_OUTPUTS}
    "${PACKET_DIR}/ServerPacketHandler.h"
)

# (3) 클라이언트 패킷 생성 스크립트 실행 (POST_BUILD)
# add_executable 이후에 위치해야 합니다.
add_custom_command(
    TARGET GameServer POST_BUILD
    COMMAND "${CLIENT_GEN_SCRIPT}"
    COMMENT "Running Client Packet Generator..."
)

# GameServer 전용 pch.h 설정
target_precompile_headers(GameServer PRIVATE pch.h)

# ServerCore 조립하기! 
target_link_libraries(GameServer PRIVATE ServerCore)

# 서드파티(Protobuf 등) 헤더 경로 추가
target_include_directories(GameServer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Proto
    ${CMAKE_CURRENT_SOURCE_DIR}/../../ThirdParty/Protobuf/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/Detour/Include
)

# Protobuf 정적 라이브러리 경로 재설정 (정확한 링크를 위해 사용 직전 정의)
if (WIN32)
    set(PROTOBUF_LIB "${CMAKE_CURRENT_SOURCE_DIR}/../../ThirdParty/Protobuf/Lib/libprotobufd.lib")
    target_link_libraries(GameServer PRIVATE ${PROTOBUF_LIB})
else()
    set(PROTOBUF_LIB "${CMAKE_CURRENT_SOURCE_DIR}/../../ThirdParty/Protobuf/Lib/libprotobuf.a")
    # 리눅스에서는 정적 라이브러리를 먼저 쓰고, 그 뒤에 시스템 라이브러리(pthread 등)를 씀
    target_link_libraries(GameServer PRIVATE ${PROTOBUF_LIB} pthread)
endif()

# 1. Resources 폴더 안의 모든 파일 목록을 싹 긁어옴
# CONFIGURE_DEPENDS 옵션: 파일이 새로 생기거나 삭제되면 CMake가 알아서 재구성(Re-configure)되게 만듦
file(GLOB_RECURSE RESOURCE_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/Resources/*")

# 2. 리소스 복사를 전담하는 가짜 타겟(Custom Target) 생성
add_custom_target(CopyResources ALL
        # 플랫폼(Windows/Linux) 상관없이 동작하는 내장 복사 명령어 사용
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/Resources"
        "${CMAKE_CURRENT_BINARY_DIR}/Resources"
        DEPENDS ${RESOURCE_FILES} # 💡 핵심: 리소스 파일들의 타임스탬프(수정 시간)가 변하면 이 타겟을 다시 실행함
        COMMENT "Syncing Resources directory..."
)

# 3. GameServer 타겟이 CopyResources 타겟에 의존(Dependency)하도록 연결
add_dependencies(GameServer CopyResources)