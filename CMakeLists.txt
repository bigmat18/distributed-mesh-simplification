cmake_minimum_required(VERSION 3.16)
project(DistributedMeshSimplification CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(LIBS_DIR "libs")
set(SRC_DIR "src")

find_package(OpenMP REQUIRED)
find_package(MPI REQUIRED)

add_subdirectory("${LIBS_DIR}/eigen")
add_subdirectory("${LIBS_DIR}/cxxopts")

set(OPENMESH_BUILD_APPS OFF CACHE BOOL "" FORCE)
set(OPENMESH_BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory("${LIBS_DIR}/OpenMesh")


file(GLOB_RECURSE CPP_FILES "${SRC_DIR}/*.cpp")

add_executable(main "${SRC_DIR}/main.cpp" ${CPP_FILES})

target_include_directories(main
    PUBLIC
        "${SRC_DIR}"
        "${LIBS_DIR}/eigen"
        SYSTEM "${LIBS_DIR}/OpenMesh/src"
        "${LIBS_DIR}/cxxopts/include"
)

target_link_libraries(main
    PUBLIC
        OpenMeshCore
        OpenMeshTools
        cxxopts
        OpenMP::OpenMP_CXX
        MPI::MPI_CXX
        stdc++exp
)

target_compile_options(main PUBLIC 
    -enable-libstdcxx-backtrace=yes
    -Wno-deprecated-enum-enum-conversion
    -Wno-unused-result
    -Wno-deprecated-declarations
)
