cmake_minimum_required(VERSION 3.10)

# 프로젝트 이름 설정
project(vktest)

# 항상 Debug 모드로 빌드하도록 설정
set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)

# compile_commands.json 생성
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# C++ 표준 설정
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Vulkan 패키지 찾기
find_package(Vulkan REQUIRED FATAL_ERROR)

# 실행 파일 생성
add_executable(vktest main.cpp)

# Vulkan include 디렉토리 추가
target_include_directories(vktest PRIVATE ${Vulkan_INCLUDE_DIRS})

# Vulkan 라이브러리 링크
target_link_libraries(vktest PRIVATE ${Vulkan_LIBRARIES})

# 추가 컴파일러 옵션 (필요한 경우)
if(MSVC)
    target_compile_options(vktest PRIVATE /W4)
else()
    target_compile_options(vktest PRIVATE -Wall -Wextra -Wpedantic)
endif()
