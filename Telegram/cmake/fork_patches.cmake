find_package(Git REQUIRED)
file(GLOB fork_ui_patches CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/../../fork-patches/*.patch")
list(SORT fork_ui_patches)
set(fork_ui_root "${CMAKE_CURRENT_SOURCE_DIR}/lib_ui")
set(fork_ui_index "${CMAKE_BINARY_DIR}/CMakeFiles/fork-ui-check.index")
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/CMakeFiles")
file(REMOVE "${fork_ui_index}")
execute_process(COMMAND "${GIT_EXECUTABLE}" apply --numstat ${fork_ui_patches}
    WORKING_DIRECTORY "${fork_ui_root}" OUTPUT_VARIABLE fork_ui_stats
    COMMAND_ERROR_IS_FATAL ANY)
string(REPLACE "\n" ";" fork_ui_rows "${fork_ui_stats}")
set(fork_ui_paths)
foreach(fork_ui_row IN LISTS fork_ui_rows)
    string(REGEX REPLACE "^[0-9-]+\t[0-9-]+\t" "" fork_ui_path "${fork_ui_row}")
    string(STRIP "${fork_ui_path}" fork_ui_path)
    if(NOT fork_ui_path STREQUAL "" AND EXISTS "${fork_ui_root}/${fork_ui_path}")
        list(APPEND fork_ui_paths "${fork_ui_path}")
    endif()
endforeach()
list(REMOVE_DUPLICATES fork_ui_paths)
set(fork_ui_git "${CMAKE_COMMAND}" -E env "GIT_INDEX_FILE=${fork_ui_index}" "${GIT_EXECUTABLE}")
execute_process(COMMAND ${fork_ui_git} read-tree HEAD WORKING_DIRECTORY "${fork_ui_root}"
    COMMAND_ERROR_IS_FATAL ANY)
execute_process(COMMAND ${fork_ui_git} add -- ${fork_ui_paths}
    WORKING_DIRECTORY "${fork_ui_root}" COMMAND_ERROR_IS_FATAL ANY)
set(fork_ui_reverse ${fork_ui_patches})
list(REVERSE fork_ui_reverse)
set(fork_ui_stack_applied TRUE)
foreach(fork_ui_patch IN LISTS fork_ui_reverse)
    execute_process(COMMAND ${fork_ui_git} apply --cached --reverse "${fork_ui_patch}"
        WORKING_DIRECTORY "${fork_ui_root}" RESULT_VARIABLE fork_ui_check
        OUTPUT_QUIET ERROR_QUIET)
    if(NOT fork_ui_check EQUAL 0)
        set(fork_ui_stack_applied FALSE)
        break()
    endif()
endforeach()
file(REMOVE "${fork_ui_index}" "${fork_ui_index}.lock")
foreach(fork_ui_patch IN LISTS fork_ui_patches)
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${fork_ui_patch}")
endforeach()
if(fork_ui_stack_applied)
    return()
endif()
foreach(fork_ui_patch IN LISTS fork_ui_patches)
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
