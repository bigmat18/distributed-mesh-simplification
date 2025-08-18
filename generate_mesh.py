import trimesh
import numpy as np
import argparse

def subdivision(mesh):
    old_vertices = mesh.vertices
    old_faces = mesh.faces

    new_vertices_list = old_vertices.tolist()
    new_faces_list = []

    edge_to_new_vertex_index = {}
    next_vertex_index = len(new_vertices_list)

    for face in old_faces:
        v0_idx, v1_idx, v2_idx = face
        midpoint_indices = []

        for i in range(3):
            v_start_idx = face[i]
            v_end_idx = face[(i + 1) % 3]
            edge = tuple(sorted((v_start_idx, v_end_idx)))

            if edge not in edge_to_new_vertex_index:
                midpoint = (old_vertices[v_start_idx] + old_vertices[v_end_idx]) / 2.0
                new_vertices_list.append(midpoint)
                edge_to_new_vertex_index[edge] = next_vertex_index
                midpoint_indices.append(next_vertex_index)
                next_vertex_index += 1
            else:
                midpoint_indices.append(edge_to_new_vertex_index[edge])

        m01_idx, m12_idx, m20_idx = midpoint_indices

        new_faces_list.append([v0_idx, m01_idx, m20_idx])
        new_faces_list.append([v1_idx, m12_idx, m01_idx])
        new_faces_list.append([v2_idx, m20_idx, m12_idx])
        new_faces_list.append([m01_idx, m12_idx, m20_idx])

    mesh.vertices = np.array(new_vertices_list, dtype=np.float64)
    mesh.faces = np.array(new_faces_list, dtype=np.int64)


def subdivide_to_target(input_file, output_file, target_triangles):
    mesh = trimesh.load(input_file, process=False)
    current_triangles = len(mesh.faces)
    print(f"Mesh {input_file} imported with {current_triangles}")

    while current_triangles < target_triangles: 
        subdivision(mesh)
        current_triangles = len(mesh.faces) 
        out_name = f"{output_file}_{current_triangles}.obj"
        mesh.export(out_name)
        print(f"Mesh saved to {out_name} with {current_triangles} triangles.")

def main():
    parser = argparse.ArgumentParser(description="Generate large meshes")
    parser.add_argument("filename", type=str, help="Input mesh filename (e.g., .obj, .ply)")
    parser.add_argument("output", type=str, help="Output mesh filename")
    parser.add_argument("target", type=int, help="Target number of triangles")
    args = parser.parse_args()

    subdivide_to_target(args.filename, args.output, args.target)

if __name__ == "__main__":
    main()
