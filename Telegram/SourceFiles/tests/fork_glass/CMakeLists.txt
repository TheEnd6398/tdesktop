cmake_minimum_required(VERSION 3.25)
project(ForkGlassChecks LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
find_package(Qt6 REQUIRED COMPONENTS Gui Widgets)
add_executable(test_glass_blur
    test_glass_blur.cpp
    ../../ui/chat/glass_blur.cpp
)
target_include_directories(test_glass_blur PRIVATE ../..)
target_link_libraries(test_glass_blur PRIVATE Qt6::Gui)
enable_testing()
add_test(NAME glass_blur COMMAND test_glass_blur)
add_executable(test_glass_material
    test_glass_material.cpp
    ../../ui/chat/glass_material.cpp
    ../../ui/chat/glass_blur.cpp
)
target_include_directories(test_glass_material PRIVATE ../..)
target_link_libraries(test_glass_material PRIVATE Qt6::Gui Qt6::Widgets)
add_test(NAME glass_material COMMAND test_glass_material)
add_executable(test_aurora_material
    test_aurora_material.cpp
    ../../ui/chat/glass_material.cpp
    ../../ui/chat/glass_blur.cpp
)
target_include_directories(test_aurora_material PRIVATE ../..)
target_link_libraries(test_aurora_material PRIVATE Qt6::Gui Qt6::Widgets)
add_test(NAME aurora_material COMMAND test_aurora_material)
add_executable(test_smooth_wheel
    test_smooth_wheel.cpp
    ../../ui/widgets/smooth_wheel_model.cpp
)
target_include_directories(test_smooth_wheel PRIVATE ../..)
target_link_libraries(test_smooth_wheel PRIVATE Qt6::Gui Qt6::Widgets)
add_test(NAME smooth_wheel COMMAND test_smooth_wheel)

add_executable(test_chat_insets test_chat_insets.cpp ../../ui/chat/chat_content_insets.cpp)
target_include_directories(test_chat_insets PRIVATE ../..)
target_link_libraries(test_chat_insets PRIVATE Qt6::Widgets)
add_test(NAME chat_insets COMMAND test_chat_insets)
