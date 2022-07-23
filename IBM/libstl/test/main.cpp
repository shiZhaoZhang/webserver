#include "stlReader.h"
#include "stdio.h"
#include <stdlib.h>
#include <libgen.h>
#include "iostream"
int main(int argc, char *argv[]){
    if(argc < 2){
        printf("Inpute %s, stdfilename\n", basename(argv[0]));
        exit(0);
    }
    try {
        stl_reader::StlMesh <float, unsigned int> mesh(argv[1]);
        for(size_t itri = 0; itri < mesh.num_tris(); ++itri) {
            std::cout << "coordinates of triangle " << itri << ": ";
            for(size_t icorner = 0; icorner < 3; ++icorner) {
                const float* c = mesh.tri_corner_coords(itri, icorner);
                // or alternatively:
                // float* c = mesh.vrt_coords (mesh.tri_corner_ind (itri, icorner));
                std::cout << "(" << c[0] << ", " << c[1] << ", " << c[2] << ") ";
            }
            std::cout << std::endl;
        
            const float* n = mesh.tri_normal(itri);
            std::cout   << "normal of triangle " << itri << ": "
                        << "(" << n[0] << ", " << n[1] << ", " << n[2] << ")\n";
        }
    }
    catch (std::exception& e) {
        std::cout << e.what() << std::endl;
    }
    return 0;
}