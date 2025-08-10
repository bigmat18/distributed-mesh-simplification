#include "utils/debug.h"
#include "utils/massert.h"
#include "utils/profiling.h"
#include <OpenMesh/Core/IO/MeshIO.hh>
#include <OpenMesh/Core/Mesh/TriMesh_ArrayKernelT.hh>

#include <unistd.h>
#include <utils/utils.h>

using Mesh = OpenMesh::TriMesh_ArrayKernelT<>;


int main(void) {
    //Mesh mesh;

    //int a = 4;
    //std::string s = "Ciao";
    //std::vector<float> v{1.0, 2.0, -0.4};

    //BREAK(VAR(a), VAR(s), VAR(v));

    //BREAK_COND(a == 4, VAR(a), VAR(s), VAR(v));

    //if (!OpenMesh::IO::read_mesh(mesh, "assets/bunny.obj")) {
        //LOG_ERROR("Error in file loading");
        //return 1;
    //}
    //LOG_INFO("File loaded");
    
    const int N = 3;

    // Parallel region
    #pragma omp parallel for
    for (int i = 0; i < N; ++i) {
        // Ogni iterazione è un "profiling scope" (può essere anche più annidato)
        PROFILING_SCOPE("LoopIteration");

        // Simula lavoro diverso per ogni thread
        std::this_thread::sleep_for(std::chrono::milliseconds(10 + i * 5));

        //// Profiling annidato (opzionale)
        //{
            //PROFILING_SCOPE("InnerWork");
            //std::this_thread::sleep_for(std::chrono::milliseconds(5));
        //}
    }

    // Stampa il profiling aggregato
    PROFILING_PRINT();


    {
        PROFILING_SCOPE("Main");
        {
            PROFILING_SCOPE("test1");
            sleep(1);

            {
                PROFILING_SCOPE("pippo");
                sleep(1);
            }
            {
                PROFILING_SCOPE("sofia");
                sleep(1);
            }
        }

        {
            PROFILING_SCOPE("test2"); 
            sleep(2);
        }
    }
    PROFILING_PRINT();


    //{
        //PROFILING_SCOPE("Main2");
        //{
            //PROFILING_SCOPE("test12");
            //sleep(1);

            //{
                //PROFILING_SCOPE("pippo2");
                //sleep(1);
            //}
            //{
                //PROFILING_SCOPE("sofia1");
                //sleep(2);
            //}
        //}

        //{
            //PROFILING_SCOPE("test3"); 
            //sleep(1);
        //}
    //}
    //PROFILING_PRINT();

    return 0;
}
