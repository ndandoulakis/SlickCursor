#ifndef KD_TREE_H
#define KD_TREE_H

#include <stack>
#include <vector>
#include <windows.h>

class KdTree {
public:
    static const int K = 2;

    KdTree() {
        data.reserve(1000);
        nodes.reserve(1000);
        validTree = false;
    }

    void addPoint(const POINT&, int userdata);
    void clear();

    void prepareSearch(const RECT&);
    bool findNext();
    int searchResult() const;

private:
    int addNode(int first, int last, int depth);

    // TODO typedef INT Point[K]; to simplify axis comparisons

    struct Data {
        POINT pt;
        int userdata;
    };

    struct Comp {
        int axis;

        Comp(int axis) : axis(axis) {}

        bool operator() (const Data & a, const Data & b) {
            return axis==0? a.pt.x < b.pt.x : a.pt.y < b.pt.y;
        }
    };

    struct Node {
        Data data;

        // positive if valid
        int axis;
        int left;
        int right;
    };

    // Tree construction
    void build();

    bool validTree;
    int count;
    std::vector<Data> data;
    std::vector<Node> nodes;

    // Range searching
    RECT range;
    int result;
    std::stack<int> iterator;

};

#endif
