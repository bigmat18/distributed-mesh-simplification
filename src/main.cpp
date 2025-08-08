#include "utils/massert.h"
#include <OpenMesh/Core/IO/MeshIO.hh>
#include <OpenMesh/Core/Mesh/TriMesh_ArrayKernelT.hh>

#include <utils/utils.h>

using Mesh = OpenMesh::TriMesh_ArrayKernelT<>;

void f() {
    ASSERT(false, "This is my first assert");
}

void g() { int a; f(); ASSERT(false, "Second assert"); }

int main(void) {
    Mesh mesh;

    g();

    if (!OpenMesh::IO::read_mesh(mesh, "assets/bunny.obj")) {
        LOG_ERROR("Error in file loading");
        return 1;
    }
    LOG_INFO("File loaded");

    return 0;
}
