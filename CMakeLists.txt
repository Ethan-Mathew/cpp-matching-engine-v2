cmake_minimum_required(VERSION 3.20)

project(CppMatchingEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(LOB_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

function(enable_sanitizers target_name)
    if (LOB_ENABLE_SANITIZERS)
        if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
            target_compile_options(${target_name} PUBLIC
                -fsanitize=address,undefined
                -fno-omit-frame-pointer
                -g
            )

            target_link_options(${target_name} PUBLIC
                -fsanitize=address,undefined
            )
        else()
            message(WARNING "Sanitizers requested, but compiler is not GCC/Clang.")
        endif()
    endif()
endfunction()

add_subdirectory(src)

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
elseif(BENCHMARKING)
    add_subdirectory(bench)
elseif(REPLAY)
    add_subdirectory(replay)
endif()