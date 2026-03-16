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

if (WIN32)
    add_custom_command(
            TARGET GameServer POST_BUILD
            COMMAND "${CLIENT_GEN_SCRIPT}"
            COMMENT "Running Client Packet Generator..."
    )
endif ()

# GameServer 전용 pch.h 설정
target_precompile_headers(GameServer PRIVATE pch.h)

# ServerCore 조립하기! 
target_link_libraries(GameServer PRIVATE ServerCore)

target_include_directories(GameServer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Proto
    ${CMAKE_CURRENT_SOURCE_DIR}/../../ThirdParty/Protobuf/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/Detour/Include
)

if (WIN32)
    set(PROTOBUF_LIB "${CMAKE_CURRENT_SOURCE_DIR}/../../ThirdParty/Protobuf/Lib/libprotobufd.lib")
    target_link_libraries(GameServer PRIVATE ${PROTOBUF_LIB})
else()
    set(PROTOBUF_LIB "${CMAKE_CURRENT_SOURCE_DIR}/../../ThirdParty/Protobuf/Lib/libprotobuf.a")
    target_link_libraries(GameServer PRIVATE ${PROTOBUF_LIB} pthread)
endif()

# 1. Resources 폴더 안의 모든 파일 목록을 싹 긁어옴
# CONFIGURE_DEPENDS 옵션: 파일이 새로 생기거나 삭제되면 CMake가 알아서 재구성(Re-configure)되게 만듦
file(GLOB_RECURSE RESOURCE_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/Resources/*")

add_custom_target(CopyResources ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/Resources"
        "${CMAKE_CURRENT_BINARY_DIR}/Resources"
        DEPENDS ${RESOURCE_FILES}
        COMMENT "Syncing Resources directory..."
)

# 3. GameServer 타겟이 CopyResources 타겟에 의존(Dependency)하도록 연결
add_dependencies(GameServer CopyResources)