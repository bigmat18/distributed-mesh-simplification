#include <OpenMesh/Core/IO/MeshIO.hh>
#include <OpenMesh/Core/Mesh/TriMesh_ArrayKernelT.hh>

#include <cstdint>
#include <cxxopts.hpp>
#include <unistd.h>
#include <utils/utils.h>

#include <Eigen/Dense>

struct Traits : public OpenMesh::DefaultTraits {
    VertexTraits { Eigen::Matrix4d quadric; };
    EdgeTraits {
        double collapse_cost;
        OpenMesh::Vec3d optimal_pos;
    };
};
using Mesh = OpenMesh::TriMesh_ArrayKernelT<Traits>;


void EvaluateVertexQuadratic() {};

void EvaluateFacePlane() {};

void EvaluateQuadraticError() {};


int main(int argc, char **argv) {
    ASSERT(argc > 1, "Need [input file]");

    cxxopts::Options options("cli", "CLI app to test distributed mesh simplification");
    options.add_options()      
        ("i,filename", "Input filename list", cxxopts::value<std::string>())
        ("n,target", "Target faces", cxxopts::value<uint32_t>());

    options.parse_positional({"filename"});
    auto result = options.parse(argc, argv);

    if(result.count("help")) {
        printf("%s", options.help().c_str()); 
        return 0;
    }

    ASSERT(result.count("filename") >= 1, "Need [input filename]");
    const std::string FILENAME        = result["filename"].as<std::string>();
    const uint32_t    TARGET_FACES    = result["target"].as<uint32_t>();

    Mesh mesh;
    ASSERT(OpenMesh::IO::read_mesh(mesh, FILENAME), "Error in mesh import");
    LOG_INFO("%s successfully imported", FILENAME.c_str());


    return 0;

}
