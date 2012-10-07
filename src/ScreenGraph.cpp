#include <math.h>
#include "screengraph.h"

using namespace std;

void ScreenGraph::analyze(Bmp24 & bmp)
{
    DWORD hash = bmp.hash();
    if (bmpHash == hash)
        return;

    bmpHash = hash;

    binarizeBmp(bmp);

    findBlobs(bmp);

    buildIndex();

    findClusters();
}

void ScreenGraph::binarizeBmp(Bmp24 & bmp)
{
    bmp.grayify();
    bmp.filter();
    bmp.binarize(bmp.threshold());
}

void ScreenGraph::findBlobs(Bmp24 & bmp)
{
    std::queue<int> empty;
    swap(queue, empty);
    explored.assign(bmp.width * bmp.height, false);

    blobs.clear();

    for (int y = 0; y < bmp.height; y++) {
        for (int x = 0; x < bmp.width; x++) {
            const int ofs = y * bmp.width + x;

            if (explored[ofs])
                continue;

            // an unexplored pixel introduces a new blob
            Blob blob;
            blob.centroid.x = 0;
            blob.centroid.y = 0;
            blob.area = 0;
            blob.bb.left = x;
            blob.bb.top = y;
            blob.bb.right = x;
            blob.bb.bottom = y;

            // BFS
            queue.push(ofs);
            explored[ofs] = true;
            while (queue.size()) {
                const int i = queue.front();
                queue.pop();

                const long xi = i % bmp.width;
                const long yi = i / bmp.width;

                blob.bb.left = min(blob.bb.left, xi);
                blob.bb.top = min(blob.bb.top, yi);
                blob.bb.right = max(blob.bb.right, xi);
                blob.bb.bottom = max(blob.bb.bottom, yi);
                blob.centroid.x += xi;
                blob.centroid.y += yi;
                blob.area++;

                // 4-connected neighborhood
                // assumimg gray-scale bmp
                // pixels are stored from the top to bottom
                BYTE * p = bmp.pixels + yi*bmp.rowLength + 3*xi;
                POINT ns[4] = {{xi-1, yi}, {xi+1, yi}, {xi, yi-1}, {xi, yi+1}};
                for (int j=0; j<4; j++) {
                    const POINT & n = ns[j];
                    BYTE * np = bmp.pixels + n.y*bmp.rowLength + 3*n.x;
                    int ni = n.y * bmp.width + n.x;
                    if (n.x>=0 && n.x<bmp.width && n.y>=0 && n.y<bmp.height && !explored[ni] && *p==*np) {
                        queue.push(ni);
                        explored[ni] = true;
                    }
                }
            }

            if (blob.area <= 1)
                continue;

            blob.centroid.x /= blob.area;
            blob.centroid.y /= blob.area;
            blob.diagonal = (long) (0.5 + sqrt(pow(blob.bb.right - blob.bb.left + 1, 2) +
                                               pow(blob.bb.bottom - blob.bb.top + 1, 2)));

            blobs.push_back(blob);
        }
    }
}

void ScreenGraph::buildIndex()
{
    index.clear();
    for (int i = 0; i < (int) blobs.size(); i++) {
        index.addPoint(blobs[i].centroid, i);
    }
}

void ScreenGraph::findClusters()
{
    std::queue<int> empty;
    swap(queue, empty);
    explored.assign(blobs.size(), false);

    clusters.clear();

    for (int i = 0; i < (int) blobs.size(); i++) {
           if (explored[i])
                continue;

            const RECT & bb = blobs[i].bb;
            Cluster c = {{bb.left, bb.top, bb.right, bb.bottom}};

            // BFS
            queue.push(i);
            explored[i] = true;
            while (queue.size()) {
                const int ia = queue.front();
                queue.pop();

                const Blob & a = blobs[ia];
                c.bb.left = min(c.bb.left, a.bb.left);
                c.bb.top = min(c.bb.top, a.bb.top);
                c.bb.right = max(c.bb.right, a.bb.right);
                c.bb.bottom = max(c.bb.bottom, a.bb.bottom);

                // TODO extract edge iteration, of a blob, into a method.
                // To save memory, edges are not stored.
                // Connected blobs are discovered on demand.
                const long d = 2*a.diagonal;
                const POINT & pt = a.centroid;
                const RECT range = {pt.x - d, pt.y - d, pt.x + d, pt.y + d};

                index.prepareSearch(range);
                while (index.findNext()) {
                    const int ib = index.searchResult();

                    if (ia == ib)
                        continue;

                    if (!explored[ib] && areConnected(ia, ib)) {
                        queue.push(ib);
                        explored[ib] = true;
                    }
                }
            }

            c.bb.right += 1;
            c.bb.bottom += 1;

            clusters.push_back(c);
    }
}

bool ScreenGraph::areConnected(int ia, int ib) const
{
    const Blob & a = blobs[ia];
    const Blob & b = blobs[ib];

    // TODO don't connect blobs with big diagonals

    // BB proportions heuristic
    if (a.diagonal > 4 * b.diagonal || b.diagonal > 4 * a.diagonal)
        return false;

    // BB distance heuristic
    if ((a.bb.left >= b.bb.right && a.bb.left - b.bb.right > 4) ||
        (a.bb.top >= b.bb.bottom && a.bb.top - b.bb.bottom > 2) ||
        (b.bb.left >= a.bb.right && b.bb.left - a.bb.right > 4) ||
        (b.bb.top >= a.bb.bottom && b.bb.top - a.bb.bottom > 2))
        return false;

    return true;
}
