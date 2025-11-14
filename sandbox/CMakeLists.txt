function(aiecad_build_sandbox)
    aiecad_auto_source(
            OUT_VAR         panels_source_files
            DIRECTORIES     "${TOP_DIR}/sandbox/panels"
            EXTENSIONS      cpp
            TRAVERSE        RECURSE
            FAIL_MODE       FATAL_ERROR
    )

    set(SANDBOX_DIR "${TOP_DIR}/sandbox")

    add_executable(panels ${panels_source_files})
    add_executable(event_bus_practice "${SANDBOX_DIR}/main.cpp")

    target_link_libraries(event_bus_practice PRIVATE aiecad_lib)
    target_link_libraries(panels PRIVATE aiecad_lib)
    
    # Add SPDLOG_FMT_EXTERNAL and /utf-8 flag for sandbox targets
    target_compile_definitions(event_bus_practice PRIVATE SPDLOG_FMT_EXTERNAL)
    target_compile_definitions(panels PRIVATE SPDLOG_FMT_EXTERNAL)
    target_compile_options(event_bus_practice PRIVATE
            $<$<CXX_COMPILER_ID:MSVC>:/utf-8>
    )
    target_compile_options(panels PRIVATE
            $<$<CXX_COMPILER_ID:MSVC>:/utf-8>
    )
endfunction()