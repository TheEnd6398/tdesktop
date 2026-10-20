cmake_minimum_required(VERSION 3.25)
project(ForkGlassChecks LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
find_package(Qt6 REQUIRED COMPONENTS Gui)
add_executable(test_glass_blur
    test_glass_blur.cpp
    ../../ui/chat/glass_blur.cpp
)
target_include_directories(test_glass_blur PRIVATE ../..)
target_link_libraries(test_glass_blur PRIVATE Qt6::Gui)
enable_testing()
add_test(NAME glass_blur COMMAND test_glass_blur)
