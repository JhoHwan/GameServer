cmake_minimum_required(VERSION 3.16)
project(DummyClient)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 소스 파일 추가 (DummyClient/Packet 폴더의 자동 생성 파일 포함)
set(PACKET_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Packet")

# 💡 중요: 구성 단계에서 폴더가 없으면 에러가 날 수 있으므로 미리 생성
if(NOT EXISTS "${PACKET_DIR}")
    file(MAKE_DIRECTORY "${PACKET_DIR}")
endif()

set(GEN_SOURCES 
    "${PACKET_DIR}/Protocol.pb.cc" 
    "${PACKET_DIR}/Protocol.pb.h"
    "${PACKET_DIR}/Struct.pb.cc"
    "${PACKET_DIR}/Struct.pb.h"
    "${PACKET_DIR}/ClientPacketHandler.h"
)

# 💡 중요: 이 파일들은 빌드 중에 생성될 것임을 CMake에 알림
set_source_files_properties(${GEN_SOURCES} PROPERTIES GENERATED TRUE)

file(GLOB_RECURSE SRC_FILES "*.cpp" "*.h")

# 소스 목록에 명시적으로 추가
add_executable(DummyClient ${SRC_FILES} ${GEN_SOURCES})

# PCH 설정
target_precompile_headers(DummyClient PRIVATE pch.h)

# 라이브러리 링크
target_link_libraries(DummyClient PRIVATE ServerCore)

# 💡 중요: GameServer가 먼저 빌드되어 패킷을 생성하도록 의존성 추가
add_dependencies(DummyClient GameServer)

if (WIN32)
    set(PROTOBUF_LIB "${CMAKE_CURRENT_SOURCE_DIR}/../../ThirdParty/Protobuf/Lib/libprotobufd.lib")
    target_link_libraries(DummyClient PRIVATE ${PROTOBUF_LIB})
else()
    set(PROTOBUF_LIB "${CMAKE_CURRENT_SOURCE_DIR}/../../ThirdParty/Protobuf/Lib/libprotobuf.a")
    target_link_libraries(DummyClient PRIVATE ${PROTOBUF_LIB} pthread)
endif()

# 인클루드 경로
target_include_directories(DummyClient PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PACKET_DIR} # 자동 생성된 패킷 헤더 경로
    ${CMAKE_CURRENT_SOURCE_DIR}/../../ThirdParty/Protobuf/Include
)
