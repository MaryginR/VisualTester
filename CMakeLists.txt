cmake_minimum_required(VERSION 3.16)
project(VisualTesterAggregate)

set(CMAKE_CXX_STANDARD 17)

# Qt setup
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(VisualTester
    main.cpp
    visualtester.cpp
    visualtester.h
    formsForTest.h
    visualtester_includes.h
    formregistry.h
    formregistry.cpp

    appconnection.h appconnection.cpp
)

target_link_libraries(VisualTester Qt6::Widgets)

target_compile_definitions(VisualTester PRIVATE VISUALTESTER_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
