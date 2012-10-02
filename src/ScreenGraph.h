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
        blobs.reserve(1000);
        edges.reserve(100000);
        clusters.reserve(1000);
    }

    void analyze(HDC dc, int x, int y, int width, int height);
    void binarizeBmp();
    void findBlobs();
    void buildIndex();
    void connectBlobs();
    void findClusters();

    struct Edge {
        // valid if positive
        long iBlob;
        long iNext;
    };

    struct Blob {
        RECT bb;
        POINT centroid;
        long area;
        long diagonal;

        // valid if positive
        long iLastEdge;
    };

    struct Cluster {
        RECT bb;
    };

    Bmp24 bmp;
    DWORD bmpHash;

    KdTree index;

    std::vector<bool> explored;

    std::vector<Edge> edges;
    std::vector<Blob> blobs;
    std::vector<Cluster> clusters;

};

#endif