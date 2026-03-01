# CMake 최소 버전 요구사항
cmake_minimum_required(VERSION 3.16)

# 프로젝트 이름과 언어(C++) 설정
project(MMOGameServer LANGUAGES CXX)

# MSVC 설정 (글깨짐 방지 및 C4819 인코딩 경고 무시)
if(MSVC)
    set(ENV{VSLANG} 1033)
    add_compile_options(/wd4819 /utf-8)
endif()

# C++ 20 표준 사용
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 하위 폴더(모듈) 추가
add_subdirectory(ServerCore)
add_subdirectory(GameServer)
add_subdirectory(DummyClient)
add_subdirectory(DummyServer)