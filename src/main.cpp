#include "OpenMesh/Core/Geometry/Vector11T.hh"
#include "OpenMesh/Core/Mesh/Handles.hh"
#include "utils/debug.h"
#include "utils/massert.h"
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
        double Error;
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

    auto Error = [&](const Eigen::Vector4d& p) { return p.transpose() * Q * p; };

    if (fabs(quadric.determinant()) > 1e-12)
        return quadric.inverse() * Eigen::Vector4d(0, 0, 0, 1);
   
    else {
        auto heh = mesh.halfedge_handle(eh, 0);
        auto vh1 = mesh.from_vertex_handle(heh);
        auto vh2 = mesh.to_vertex_handle(heh);
        Eigen::Vector4d p1(mesh.point(vh1)[0], mesh.point(vh1)[1], mesh.point(vh1)[2], 1.f);
        Eigen::Vector4d p2(mesh.point(vh2)[0], mesh.point(vh2)[1], mesh.point(vh2)[2], 1.f);
        Eigen::Vector4d mid = 0.5 * (p1 + p2);

        double e1 = Error(p1);
        double e2 = Error(p2);
        double em = Error(mid);

        if (e1 <= e2 && e1 <= em)       return p1;
        else if (e2 <= e1 && e2 <= em)  return p2;
        else                            return mid;
    }
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
    mesh.request_vertex_status();
    mesh.request_edge_status();
    mesh.request_face_status();
    mesh.request_halfedge_status();


    
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
        auto heh = mesh.halfedge_handle(eh, 0);
        auto v0 = mesh.from_vertex_handle(heh);
        auto v1 = mesh.to_vertex_handle(heh);

        Eigen::Matrix4d Q = mesh.data(v0).Quadric + mesh.data(v1).Quadric;
        Eigen::Vector4d newV = EvaluateNewBestVertex(mesh, eh, Q);

        mesh.data(eh).Error = newV.transpose() * Q * newV;
        mesh.data(eh).NewVertex = newV;
        pq.push(eh);
    }

    //while (!pq.empty()) {
        //auto eh = pq.top();
        //pq.pop();

        //auto heh = mesh.halfedge_handle(eh, 0);
        //auto vh0 = mesh.from_vertex_handle(heh);
        //auto vh1 = mesh.to_vertex_handle(heh);

        //OpenMesh::Vec3f c0 = mesh.point(vh0);
        //OpenMesh::Vec3f c1 = mesh.point(vh1);
        //Eigen::Vector4d nc = mesh.data(eh).NewVertex;
        //std::cout << "V0: (" << c0[0] << "," << c0[1] << "," << c0[2] << 
                  //") V1: (" << c1[0] << "," << c1[1] << "," << c1[2] <<
                  //") newVetex: (" << nc.x() << "," << nc.y() << "," << nc.z() << ")" << std::endl;
    //}
  

    while (mesh.n_faces() > TARGET_FACES && !pq.empty()) {
        auto eh = pq.top();
        pq.pop();

        if (mesh.status(eh).deleted()) {
            BREAK();
            continue;
        }

        auto heh = mesh.halfedge_handle(eh, 0);

        if (!mesh.is_collapse_ok(heh)) {BREAK(); continue;}
        
        auto vh_to_keep = mesh.to_vertex_handle(heh);
        Eigen::Vector4d newVertex = mesh.data(eh).NewVertex;
        OpenMesh::Vec3f coords(newVertex.x(), newVertex.y(), newVertex.z());

        mesh.set_point(vh_to_keep, coords);
        mesh.collapse(heh);

        break;
    }
    mesh.garbage_collection();

    std::cout << "Dopo il collasso: " 
              << mesh.n_edges() << " edges, " 
              << mesh.n_faces() << " faces\n";

    ASSERT(OpenMesh::IO::write_mesh(mesh, "out/out.obj"), "Error in mesh export!");
    LOG_INFO("Mesh successfully exported!");
    return 0;

} 
