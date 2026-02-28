cmake_minimum_required(VERSION 3.20)
project(DummyServer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 소스 파일 추가
file(GLOB_RECURSE SRC_FILES 
    "*.cpp" 
    "*.h"
)

add_executable(DummyServer ${SRC_FILES})

# ServerCore 라이브러리 링크 및 Include 디렉토리 설정
target_link_libraries(DummyServer PRIVATE ServerCore)
