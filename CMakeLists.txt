cmake_minimum_required(VERSION 3.16)
project(invisibility_cloak LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED)

add_executable(${PROJECT_NAME}
    main.cpp
    Track.cpp
    Invisible.cpp
    PersonSeg.cpp
)

add_executable(seg_test seg_test.cpp)
target_link_libraries(seg_test PRIVATE ${OpenCV_LIBS})

if(MSVC)
    target_compile_options(seg_test PRIVATE /utf-8)
endif()

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE ${OpenCV_LIBS})
target_compile_definitions(${PROJECT_NAME} PRIVATE NOMINMAX)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /utf-8 /W4)
endif()
