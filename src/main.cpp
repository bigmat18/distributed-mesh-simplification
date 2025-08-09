#include "utils/debug.h"
#include "utils/massert.h"
#include <OpenMesh/Core/IO/MeshIO.hh>
#include <OpenMesh/Core/Mesh/TriMesh_ArrayKernelT.hh>

#include <utils/utils.h>

using Mesh = OpenMesh::TriMesh_ArrayKernelT<>;


int main(void) {
    Mesh mesh;

    int a = 4;
    std::string s = "Ciao";
    std::vector<float> v{1.0, 2.0, -0.4};

    BREAK(VAR(a), VAR(s), VAR(v));

    BREAK_COND(a == 4, VAR(a), VAR(s), VAR(v));

    if (!OpenMesh::IO::read_mesh(mesh, "assets/bunny.obj")) {
        LOG_ERROR("Error in file loading");
        return 1;
    }
    LOG_INFO("File loaded");

    return 0;
}
