file(GLOB_RECURSE MAIN_SOURCES "Main/*.cpp" "Main/*.h")
file(GLOB_RECURSE THREAD_SOURCES "Thread/*.cpp" "Thread/*.h")
file(GLOB_RECURSE BUFFER_SOURCES "Buffer/*.cpp" "Buffer/*.h")
file(GLOB_RECURSE NETWORK_SOURCES "Network/*.cpp" "Network/*.h")
file(GLOB_RECURSE UTIL_SOURCES "Util/*.cpp" "Util/*.h")
file(GLOB_RECURSE DB_SOURCES "DB/*.cpp" "DB/*.h")

if(WIN32)
    # 윈도우 빌드 시 Windows 폴더만 포함
    file(GLOB_RECURSE PLATFORM_SOURCES "Platform/Windows/*.cpp" "Platform/Windows/*.h")
    set(PLATFORM_INCLUDE_DIR "Platform/Windows")
else()
    # 리눅스 빌드 시 Linux 폴더만 포함
    file(GLOB_RECURSE PLATFORM_SOURCES "Platform/Linux/*.cpp" "Platform/Linux/*.h")
    set(PLATFORM_INCLUDE_DIR "Platform/Linux")
endif()

add_library(ServerCore STATIC 
    ${MAIN_SOURCES} 
    ${THREAD_SOURCES} 
    ${BUFFER_SOURCES} 
    ${NETWORK_SOURCES} 
    ${UTIL_SOURCES}
    ${DB_SOURCES}
    ${PLATFORM_SOURCES}
)

target_include_directories(ServerCore PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/Main
    ${CMAKE_CURRENT_SOURCE_DIR}/Thread
    ${CMAKE_CURRENT_SOURCE_DIR}/Buffer
    ${CMAKE_CURRENT_SOURCE_DIR}/Network
    ${CMAKE_CURRENT_SOURCE_DIR}/Util
    ${CMAKE_CURRENT_SOURCE_DIR}/DB
    ${CMAKE_CURRENT_SOURCE_DIR}/${PLATFORM_INCLUDE_DIR}
)

# WSL/GCC 환경 호환성을 위해 PRIVATE으로 변경 (PUBLIC 상속 시 경로 문제 발생)
target_precompile_headers(ServerCore PRIVATE Main/pch.h)

if(WIN32)
    target_link_libraries(ServerCore PUBLIC ws2_32 mswsock)
    target_compile_definitions(ServerCore PUBLIC WIN32_LEAN_AND_MEAN NOMINMAX)
else()
    find_package(ODBC REQUIRED)
    target_include_directories(ServerCore PRIVATE ${ODBC_INCLUDE_DIR})
    target_link_libraries(ServerCore PRIVATE ${ODBC_LIBRARY})

    target_link_libraries(ServerCore PUBLIC pthread)
    # CONFIG가 Debug일 때만 뒤의 옵션들을 전개함
    target_compile_options(ServerCore PUBLIC
            $<$<CONFIG:Debug>:-fsanitize=address>
            $<$<CONFIG:Debug>:-g>
            $<$<CONFIG:Debug>:-fno-omit-frame-pointer>
    )
    target_link_options(ServerCore PUBLIC
            $<$<CONFIG:Debug>:-fsanitize=address>
    )
endif()