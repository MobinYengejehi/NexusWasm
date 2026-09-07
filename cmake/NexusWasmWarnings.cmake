function(nexuswasm_enable_warnings target)

    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")

        target_compile_options(
            ${target}
            PRIVATE
            /W4
            /permissive-
        )

        if(NEXUSWASM_WARNINGS_AS_ERRORS)
            target_compile_options(
                ${target}
                PRIVATE
                /WX
            )
        endif()

    elseif(
        CMAKE_CXX_COMPILER_ID STREQUAL "Clang"
        AND
        CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC"
    )

        target_compile_options(
            ${target}
            PRIVATE
            /W4
        )

        if(NEXUSWASM_WARNINGS_AS_ERRORS)
            target_compile_options(
                ${target}
                PRIVATE
                /WX
            )
        endif()

    elseif(
        CMAKE_CXX_COMPILER_ID MATCHES
        "GNU|Clang|AppleClang"
    )

        target_compile_options(
            ${target}
            PRIVATE
            -Wall
            -Wextra
            -Wpedantic
        )

        if(NEXUSWASM_WARNINGS_AS_ERRORS)
            target_compile_options(
                ${target}
                PRIVATE
                -Werror
            )
        endif()

    endif()

endfunction()
