cmake_minimum_required (VERSION 2.8.11)
project (MAESTRO)

include_directories(lib/include/)

find_package(Boost)
IF (Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIR})
endif()


add_executable (cmake_maestro maestro-top.cpp
        lib/src/maestro.cpp
        )
