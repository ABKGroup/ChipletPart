#include <cstddef> /* NULL */
#include <metis.h>
#include <iostream>
#include <vector>


// Install metis from:
// http://glaros.dtc.umn.edu/gkhome/fetch/sw/metis/metis-5.1.0.tar.gz

// Build with
// g++ metis.cc -lmetis

int main(){

    idx_t nVertices = 6;
    idx_t nEdges    = 7;
    idx_t nWeights  = 1;
    idx_t nParts    = 2;

    idx_t objval;
    std::vector<idx_t> part(nVertices, 0);


    // Indexes of starting points in adjacent array
    std::vector<idx_t> xadj = {0,2,5,7,9,12,14};

    // Adjacent vertices in consecutive index order
    std::vector<idx_t> adjncy = {1,3,0,4,2,1,5,0,4,3,1,5,4,2};

    // Weights of vertices
    // if all weights are equal then can be set to NULL
    std::vector<idx_t> vwgt(nVertices * nWeights, 0);
    


    int ret = METIS_PartGraphKway(&nVertices,& nWeights, xadj.data(), adjncy.data(),
				       NULL, NULL, NULL, &nParts, NULL,
       				  NULL, NULL, &objval, part.data());

    std::cout << ret << std::endl;
    
    for(unsigned part_i = 0; part_i < part.size(); part_i++){
	std::cout << part_i << " " << part[part_i] << std::endl;
    }

    
    return 0;
}