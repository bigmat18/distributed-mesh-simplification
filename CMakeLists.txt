cmake_minimum_required(VERSION 3.16)
project(DistributedMeshSimplification CXX)

set(CMAKE_CXX_STANDARD 20)
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

add_executable(main "${SRC_DIR}/main.cpp")

target_include_directories(main
    PRIVATE
        "${LIBS_DIR}/eigen"
        "${LIBS_DIR}/OpenMesh/src"
        "${LIBS_DIR}/cxxopts/include"
)


target_link_libraries(main
    PRIVATE
        OpenMeshCore
        OpenMeshTools
        cxxopts
        OpenMP::OpenMP_CXX
        MPI::MPI_CXX
)

