#include "OpenMesh/Core/Mesh/Handles.hh"
#include <OpenMesh/Core/IO/MeshIO.hh>
#include <OpenMesh/Core/Mesh/TriMesh_ArrayKernelT.hh>

#include <cstdint>
#include <cxxopts.hpp>
#include <iterator>
#include <ostream>
#include <queue>
#include <unistd.h>
#include <utils/utils.h>

#include <Eigen/Dense>

struct Traits : public OpenMesh::DefaultTraits {
    VertexTraits { 
        Eigen::Matrix4d Quadric; 
    };

    EdgeTraits { 
        float Error;
        Eigen::Vector4d NewVertex;
    };
};

using Mesh = OpenMesh::TriMesh_ArrayKernelT<Traits>;



inline Eigen::Vector4d EvaluateFacePlane(Mesh& mesh, 
                                         const OpenMesh::FaceHandle fh) 
{

    std::array<Eigen::Vector3f, 3> points;
    int i = 0;
    for (auto fv_it = mesh.fv_iter(fh); fv_it.is_valid(); ++fv_it) {
        auto p = mesh.point(*fv_it);
        points[i++] = Eigen::Vector3f(p[0], p[1], p[2]);
    }

    Eigen::Vector3f u = points[1] - points[0];
    Eigen::Vector3f v = points[2] - points[0];
    Eigen::Vector3f n = u.cross(v);
    n.normalize();

    float a = n.x();
    float b = n.y();
    float c = n.z();
    float d = -n.dot(points[0]);

    return Eigen::Vector4d(a, b, c, d);
}

inline Eigen::Matrix4d EvaluateFacePlaneMatrix(Mesh& mesh, 
                                               const OpenMesh::FaceHandle fh)
{
    Eigen::Vector4d planeCoeficient = EvaluateFacePlane(mesh, fh);
    return planeCoeficient * planeCoeficient.transpose();
}

inline Eigen::Matrix4d EvaluateVertexQuadratic(Mesh& mesh, 
                                               const OpenMesh::VertexHandle vh)
{
    Eigen::Matrix4d result = Eigen::Matrix4d::Zero();
    for (auto f_it = mesh.vf_iter(vh); f_it.is_valid(); ++f_it) {
        auto fh = *f_it; 
        result += EvaluateFacePlaneMatrix(mesh, fh);        
    }

    return result;
}

inline Eigen::Vector4d EvaluateNewBestVertex(const Mesh& mesh, 
                                             const OpenMesh::EdgeHandle eh, 
                                             const Eigen::Matrix4d Q) 
{   
    Eigen::Matrix4d quadric;
    quadric << Q(0,0), Q(0,1), Q(0,2), Q(0,3),
               Q(0,1), Q(1,1), Q(1,2), Q(1,3),
               Q(0,2), Q(1,2), Q(2,2), Q(2,3),
               0.0f,   0.0f,   0.0f,   1.0f;

    if (fabs(quadric.determinant()) > 1e-6f) {
        auto error = [&](const Eigen::Vector4d& p) { return p.transpose() * Q * p; };

        auto heh = mesh.halfedge_handle(eh, 0);
        auto vh1 = mesh.from_vertex_handle(heh);
        auto vh2 = mesh.to_vertex_handle(heh);

        auto coords1 = mesh.point(vh1);
        auto coords2 = mesh.point(vh2);
    
        Eigen::Vector4d p1 = Eigen::Vector4d(coords1[0], coords1[1], coords1[2], 1);
        Eigen::Vector4d p2 = Eigen::Vector4d(coords2[0], coords2[1], coords2[2], 1);
    
        Eigen::Vector4d mid = 0.5f * (p1 + p2);

        float e1 = error(p1);
        float e2 = error(p2);
        float em = error(mid);

        if      (e1 <= e2 && e1 <= em)  return p1;
        else if (e2 <= e1 && e2 <= em)  return p2;
        else                            return mid;
    }

    return quadric.inverse() * Eigen::Vector4d(0.f, 0.f, 0.f, 1.f);
}



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


    
    for (auto v_it = mesh.vertices_begin(); v_it != mesh.vertices_end(); ++v_it) {
        const auto vh = *v_it;
        mesh.data(vh).Quadric = EvaluateVertexQuadratic(mesh, vh);
    }

     
    auto cmp = [&](const Mesh::EdgeHandle& e1, const Mesh::EdgeHandle& e2) {
        return mesh.data(e1).Error > mesh.data(e2).Error;
    };

    std::priority_queue<
        Mesh::EdgeHandle,
        std::vector<Mesh::EdgeHandle>,
        decltype(cmp)
    > pq(cmp);


    for (auto e_it = mesh.edges_begin(); e_it != mesh.edges_end(); ++e_it) {
        auto eh = *e_it;
        Eigen::Matrix4d Q0 = mesh.data(eh.v0()).Quadric;
        Eigen::Matrix4d Q1 = mesh.data(eh.v1()).Quadric;
        Eigen::Matrix4d Q = Q0 + Q1;
        Eigen::Vector4d newV = EvaluateNewBestVertex(mesh, eh, Q);

        mesh.data(eh).Error = newV.transpose() * Q * newV;
        mesh.data(eh).NewVertex = newV;
        pq.push(eh);
    }


   
    while (!pq.empty()) {
        auto eh = pq.top();
        pq.pop();
        std::cout << "Edge " << eh.idx()
                  << " errore: " << mesh.data(eh).Error << "\n";
    }
    //while (mesh.n_faces() > TARGET_FACES) {
        //auto eh = pq.top();
    //}

    return 0;

} 
