# 현재 폴더에 있는 모든 .cpp, .h 파일을 찾아서 CORE_SOURCES 변수에 담기
file(GLOB_RECURSE CORE_SOURCES "*.cpp" "*.h")

# ServerCore라는 이름의 정적 라이브러리(부품) 만들기
add_library(ServerCore STATIC ${CORE_SOURCES})

# 이 라이브러리를 쓰는 쪽에서 ServerCore의 헤더 파일을 바로 찾을 수 있게 경로 지정
target_include_directories(ServerCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# 미리 컴파일된 헤더(pch.h) 세팅 (VS의 복잡한 설정을 한 줄로 끝냄!)
target_precompile_headers(ServerCore PUBLIC pch.h)

# 윈도우 환경일 경우 소켓 프로그래밍에 필요한 윈도우 기본 라이브러리(ws2_32) 연결
if(WIN32)
    target_link_libraries(ServerCore PUBLIC ws2_32 mswsock)
    target_compile_definitions(ServerCore PUBLIC WIN32_LEAN_AND_MEAN NOMINMAX)
endif()