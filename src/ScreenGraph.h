#ifndef SCREENGRAPH_H
#define SCREENGRAPH_H

#include <queue>
#include <vector>
#include "bmp24.h"
#include "kdtree.h"

// ScreenGraph nodes correspond to bmp blobs.
// 4-connected pixels of the same color belong to the same blob.
// Blobs are linked together (connected) using a couple of heuristics.
// Clustered blobs are connected components (subgraphs) of ScreenGraph.

struct ScreenGraph {
    ScreenGraph() {
    }

    void analyze(Bmp24&);
    void binarizeBmp(Bmp24&);
    void findBlobs(Bmp24&);
    void buildIndex();
    void findClusters();
    bool areConnected(int, int) const;

    struct Blob {
        RECT bb;
        POINT centroid;
        long area;
        long diagonal;
    };

    struct Cluster {
        RECT bb;
    };

    DWORD bmpHash;

    KdTree index;

    std::vector<bool> explored;
    std::queue<int> queue;

    std::vector<Blob> blobs;
    std::vector<Cluster> clusters;

};

#endif
