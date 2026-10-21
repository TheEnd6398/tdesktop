find_package(Git REQUIRED)
file(GLOB fork_ui_patches CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/../../fork-patches/*.patch")
list(SORT fork_ui_patches)
foreach(fork_ui_patch IN LISTS fork_ui_patches)
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${fork_ui_patch}")
    execute_process(COMMAND "${GIT_EXECUTABLE}" apply --reverse --check "${fork_ui_patch}"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib_ui"
        RESULT_VARIABLE fork_patch_applied OUTPUT_QUIET ERROR_QUIET)
    if(NOT fork_patch_applied EQUAL 0)
        execute_process(COMMAND "${GIT_EXECUTABLE}" apply --check "${fork_ui_patch}"
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib_ui"
            RESULT_VARIABLE fork_patch_check ERROR_VARIABLE fork_patch_error)
        if(NOT fork_patch_check EQUAL 0)
            message(FATAL_ERROR "Cannot apply fork UI patch ${fork_ui_patch}: ${fork_patch_error}")
        endif()
        execute_process(COMMAND "${GIT_EXECUTABLE}" apply "${fork_ui_patch}"
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib_ui"
            COMMAND_ERROR_IS_FATAL ANY)
    endif()
endforeach()
