#include <algorithm>
#include "kdtree.h"

void KdTree::addPoint(const POINT & pt, int userdata)
{
    Data d = {{pt.x, pt.y}, userdata};
    data.push_back(d);
    validTree = false;
}

void KdTree::clear()
{
    data.clear();
    validTree = false;
}

void KdTree::build()
{
    count = 0;
    nodes.resize(data.size());
    validTree = addNode(0, data.size(), 0) >= 0;
}

int KdTree::addNode(int first, int last, int depth)
{
    // range [first, last)

    if (first >= last)
        return -1;

    const int axis = depth % K;
    const int median = (first + last) / 2;

    std::nth_element(data.begin() + first,
                     data.begin() + median,
                     data.begin() + last,
                     Comp(axis));

    const int index = count++;

    Node & n = nodes[index];
    n.axis = axis;
    n.data = data[median];
    n.left = addNode(first, median, depth + 1);
    n.right = addNode(median + 1, last, depth + 1);

    return index;
}

void KdTree::prepareSearch(const RECT & r)
{
    if (!validTree)
        build();

    std::stack<int> empty;
    std::swap(iterator, empty);

    if (data.size()) {
        iterator.push(0);

        range = r;
        // The range is inclusive and since findNext() considers
        // bottom/right sides outside range, we must adjust them.
        range.right++;
        range.bottom++;
    }
}

bool KdTree::findNext()
{
    // non-recursive DFS
    while (!iterator.empty()) {
        const Node & n = nodes[iterator.top()];
        iterator.pop();

        const POINT & pt = n.data.pt;

        if ((n.axis==0 && pt.x<range.right) || (n.axis==1 && pt.y<range.bottom))
            if (n.right>=0) iterator.push(n.right);

        if ((n.axis==0 && pt.x>=range.left) || (n.axis==1 && pt.y>=range.top))
            if (n.left>=0) iterator.push(n.left);

        if (PtInRect(&range, pt)) {
            result = n.data.userdata;
            return true;
        }
    }

    return false;
}

int KdTree::searchResult() const
{
    return result;
}
