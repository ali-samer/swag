if (LINUX)
    message(FATAL_ERROR "This project is based on programming AMD's NPU AIE-ML chip, thus it is not
        supported for non Linux devices.")
endif ()

if (NOT DEFINED IS_AARCH64_ARCH AND ${CMAKE_SYSTEM_PROCESSOR} MATCHES "^(aarch64|arm64|AMD64)$")
    set(IS_AARCH64_ARCH TRUE)
endif ()

if (NOT DEFINED IS_X86_64_ARCH AND ${CMAKE_SYSTEM_PROCESSOR} MATCHES "^(x86_64|AMD64)")
    set(IS_X86_64_ARCH TRUE)
endif ()

if (DEFINED IS_X86_64_ARCH OR DEFINED IS_AARCH64_ARCH)
    message(STATUS "Architecture detected: ${CMAKE_SYSTEM_PROCESSOR}")
else ()
    message(FATAL_ERROR "Archicture detected is not supported: ${CMAKE_SYSTEM_PROCESSOR}")
endif ()