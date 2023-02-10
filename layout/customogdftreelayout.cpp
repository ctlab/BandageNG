#include "customogdftreelayout.h"
#include <ogdf/basic/AdjEntryArray.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/basic/LayoutModule.h>
#include "ogdf/basic/Math.h"
#include <ogdf/basic/SList.h>

CustomOgdfTreeLayout::CustomOgdfTreeLayout()
    :m_siblingDistance(20),
     m_subtreeDistance(20),
     m_levelDistance(50),
     m_treeDistance(50),
     m_orthogonalLayout(false),
     m_orientation(ogdf::Orientation::topToBottom),
     m_selectRoot(RootSelectionType::Source)
{ }

struct CustomOgdfTreeLayout::TreeStructure {

    ogdf::GraphAttributes &m_ga;
    ogdf::NodeArray<int> m_number;         //!< Consecutive numbers for children.

    ogdf::NodeArray<ogdf::node> m_parent;        //!< Parent node, 0 if root.
    ogdf::NodeArray<ogdf::node> m_leftSibling;   //!< Left sibling, 0 if none.
    ogdf::NodeArray<ogdf::node> m_firstChild;    //!< Leftmost child, 0 if leaf.
    ogdf::NodeArray<ogdf::node> m_lastChild;	 //!< Rightmost child, 0 if leaf.
    ogdf::NodeArray<ogdf::node> m_thread;        //!< Thread, 0 if none.
    ogdf::NodeArray<ogdf::node> m_ancestor;      //!< Actual highest ancestor.

    ogdf::NodeArray<double> m_preliminary; //!< Preliminary x-coordinates.
    ogdf::NodeArray<double> m_modifier;    //!< Modifier of x-coordinates.
    ogdf::NodeArray<double> m_change;      //!< Change of shift applied to subtrees.
    ogdf::NodeArray<double> m_shift;       //!< Shift applied to subtrees.


    // initialize all node arrays and
    // compute the tree structure from the adjacency lists
    //
    // returns the root nodes in roots
    TreeStructure(const ogdf::Graph &tree, ogdf::GraphAttributes &GA, ogdf::List<ogdf::node> &roots) :
        m_ga(GA),
        m_number(tree, 0),
        m_parent(tree, nullptr),
        m_leftSibling(tree, nullptr),
        m_firstChild(tree, nullptr),
        m_lastChild(tree, nullptr),
        m_thread(tree, nullptr),
        m_ancestor(tree, nullptr),
        m_preliminary(tree, 0),
        m_modifier(tree, 0),
        m_change(tree, 0),
        m_shift(tree, 0)
    {
        // compute the tree structure

        // find the roots
        //node root = 0;
        for (ogdf::node v : tree.nodes) {
            if (v->indeg() == 0)
                roots.pushBack(v);
        }

        int childCounter;
        for (ogdf::node v : tree.nodes) {

            // determine
            // - the parent node of v
            // - the leftmost and rightmost child of v
            // - the numbers of the children of v
            // - the left siblings of the children of v
            // and initialize the actual ancestor of v

            m_ancestor[v] = v;
            if (isLeaf(v)) {
                if (v->indeg() == 0) { // is v a root
                    m_parent[v] = nullptr;
                    m_leftSibling[v] = nullptr;
                }
                else {
                    m_firstChild[v] = m_lastChild[v] = nullptr;
                    m_parent[v] = v->firstAdj()->theEdge()->source();
                }
            }
            else {

                // traverse the adjacency list of v
                ogdf::adjEntry first;    // first leaving edge
                ogdf::adjEntry stop;     // successor of last leaving edge
                first = v->firstAdj();
                if (v->indeg() == 0) { // is v a root
                    stop = first;
                    m_parent[v] = nullptr;
                    m_leftSibling[v] = nullptr;
                }
                else {

                    // search for first leaving edge
                    while (first->theEdge()->source() == v)
                        first = first->cyclicSucc();
                    m_parent[v] = first->theEdge()->source();
                    stop = first;
                    first = first->cyclicSucc();
                }

                // traverse the children of v
                m_firstChild[v] = first->theEdge()->target();
                m_number[m_firstChild[v]] = childCounter = 0;
                m_leftSibling[m_firstChild[v]] = nullptr;
                ogdf::adjEntry previous = first;
                while (first->cyclicSucc() != stop) {
                    first = first->cyclicSucc();
                    m_number[first->theEdge()->target()] = ++childCounter;
                    m_leftSibling[first->theEdge()->target()]
                        = previous->theEdge()->target();
                    previous = first;
                }
                m_lastChild[v] = first->theEdge()->target();
            }
        }
    }

    // returns whether node v is a leaf
    bool isLeaf(ogdf::node v) const
    {
        OGDF_ASSERT(v != nullptr);

        // node v is a leaf if and only if no edge leaves v
        return v->outdeg() == 0;
    }

    // returns the successor of node v on the left contour
    // returns 0 if there is none
    ogdf::node nextOnLeftContour(ogdf::node v) const
    {
        OGDF_ASSERT(v != nullptr);
        OGDF_ASSERT(v->graphOf() == m_firstChild.graphOf());
        OGDF_ASSERT(v->graphOf() == m_thread.graphOf());

        // if v has children, the successor of v on the left contour
        // is its leftmost child,
        // otherwise, the successor is the thread of v (may be 0)
        if (m_firstChild[v] != nullptr)
            return m_firstChild[v];
        else
            return m_thread[v];
    }

    // returns the successor of node v on the right contour
    // returns 0 if there is none
    ogdf::node nextOnRightContour(ogdf::node v) const
    {
        OGDF_ASSERT(v != nullptr);
        OGDF_ASSERT(v->graphOf() == m_lastChild.graphOf());
        OGDF_ASSERT(v->graphOf() == m_thread.graphOf());

        // if v has children, the successor of v on the right contour
        // is its rightmost child,
        // otherwise, the successor is the thread of v (may be 0)
        if (m_lastChild[v] != nullptr)
            return m_lastChild[v];
        else
            return m_thread[v];
    }

};

void CustomOgdfTreeLayout::call(ogdf::GraphAttributes &AG)
{
 callMultiLine(AG, 1);
}

void CustomOgdfTreeLayout::callMultiLine(ogdf::GraphAttributes &AG, int numOfRows)
{
    const ogdf::Graph &tree = AG.constGraph();
    if(tree.numberOfNodes() == 0) return;

    OGDF_ASSERT(isArborescenceForest(tree));
    OGDF_ASSERT(m_siblingDistance > 0);
    OGDF_ASSERT(m_subtreeDistance > 0);
    OGDF_ASSERT(m_levelDistance > 0);

    // compute the tree structure
    ogdf::List<ogdf::node> roots;
    TreeStructure ts(tree, AG, roots);

    int numInRow;
    if (numOfRows != 0) {
        numInRow = std::max(roots.size() / numOfRows, 1);
    }
    else {
        numInRow = sqrt(roots.size());
    }

    double minX = 0, minY = 0, maxX = 0, maxY = 0, yShift = 0;
    int curNumInRow = 0;
    for(ogdf::ListConstIterator<ogdf::node> it = roots.begin(); it.valid(); ++it)
    {
        ogdf::node root = *it;

        // compute x-coordinates
        firstWalk(ts, root, true);
        secondWalkX(ts, root, -ts.m_preliminary[root]);

        // compute y-coordinates
        computeYCoordinatesAndEdgeShapes(root,AG);

        bool firstInNewLine = false;
        if (curNumInRow + 1 > numInRow) {
            firstInNewLine = true;
            curNumInRow = 0;
            yShift = maxY;
            minX = 0;
            maxX = 0;
        }

        if (it == roots.begin()) {
            firstInNewLine = true;
        }

        findMinY(AG, root, minY);
        double shift = yShift + m_treeDistance - minY;
        shiftTreeY(AG, root, shift);

        if (!firstInNewLine)
        {
            findMinX(AG, root, minX);

            double shift = maxX + m_treeDistance - minX;

            shiftTreeX(AG, root, shift);
        }

        findMaxX(AG, root, maxX);
        findMaxY(AG, root, maxY);
        curNumInRow += 1;
    }

}

void CustomOgdfTreeLayout::findMinX(ogdf::GraphAttributes &AG, ogdf::node root, double &minX)
{
    ogdf::ArrayBuffer<ogdf::node> S;
    S.push(root);

    while(!S.empty())
    {
        ogdf::node v = S.popRet();

        double left = AG.x(v) - AG.width(v)/2;
        if(left < minX) minX = left;

        for(ogdf::adjEntry adj : v->adjEntries) {
            ogdf::edge e = adj->theEdge();
            ogdf::node w = e->target();
            if(w != v) S.push(w);
        }
    }
}

void CustomOgdfTreeLayout::findMinY(ogdf::GraphAttributes &AG, ogdf::node root, double &minY)
{
    ogdf::ArrayBuffer<ogdf::node> S;
    S.push(root);

    while(!S.empty())
    {
        ogdf::node v = S.popRet();

        double left = AG.y(v) - AG.height(v)/2;
        if(left < minY) minY = left;

        for(ogdf::adjEntry adj : v->adjEntries) {
            ogdf::edge e = adj->theEdge();
            ogdf::node w = e->target();
            if(w != v) S.push(w);
        }
    }
}


void CustomOgdfTreeLayout::shiftTreeX(ogdf::GraphAttributes &AG, ogdf::node root, double shift)
{
    ogdf::ArrayBuffer<ogdf::node> S;
    S.push(root);
    while(!S.empty())
    {
        ogdf::node v = S.popRet();

        AG.x(v) += shift;

        for(ogdf::adjEntry adj : v->adjEntries) {
            ogdf::edge e = adj->theEdge();
            ogdf::node w = e->target();
            if(w != v) {
                ogdf::ListIterator<ogdf::DPoint> itP;
                for(itP = AG.bends(e).begin(); itP.valid(); ++itP)
                    (*itP).m_x += shift;
                S.push(w);
            }
        }
    }
}

void CustomOgdfTreeLayout::shiftTreeY(ogdf::GraphAttributes &AG, ogdf::node root, double shift)
{
    ogdf::ArrayBuffer<ogdf::node> S;
    S.push(root);
    while(!S.empty())
    {
        ogdf::node v = S.popRet();

        AG.y(v) += shift;

        for(ogdf::adjEntry adj : v->adjEntries) {
            ogdf::edge e = adj->theEdge();
            ogdf::node w = e->target();
            if(w != v) {
                ogdf::ListIterator<ogdf::DPoint> itP;
                for(itP = AG.bends(e).begin(); itP.valid(); ++itP)
                    (*itP).m_y += shift;
                S.push(w);
            }
        }
    }
}


void CustomOgdfTreeLayout::findMaxX(ogdf::GraphAttributes &AG, ogdf::node root, double &maxX)
{
    ogdf::ArrayBuffer<ogdf::node> S;
    S.push(root);
    while(!S.empty())
    {
        ogdf::node v = S.popRet();

        double right = AG.x(v) + AG.width(v)/2;
        if(right > maxX) maxX = right;

        for(ogdf::adjEntry adj : v->adjEntries) {
            ogdf::edge e = adj->theEdge();
            ogdf::node w = e->target();
            if(w != v) S.push(w);
        }
    }
}

void CustomOgdfTreeLayout::findMaxY(ogdf::GraphAttributes &AG, ogdf::node root, double &maxY)
{
    ogdf::ArrayBuffer<ogdf::node> S;
    S.push(root);
    while(!S.empty())
    {
        ogdf::node v = S.popRet();

        double right = AG.y(v) + AG.height(v)/2;
        if(right > maxY) maxY = right;

        for(ogdf::adjEntry adj : v->adjEntries) {
            ogdf::edge e = adj->theEdge();
            ogdf::node w = e->target();
            if(w != v) S.push(w);
        }
    }
}


void CustomOgdfTreeLayout::firstWalk(TreeStructure &ts, ogdf::node subtree, bool upDown)
{
    OGDF_ASSERT(subtree != nullptr);
    OGDF_ASSERT(subtree->graphOf() == ts.m_leftSibling.graphOf());
    OGDF_ASSERT(subtree->graphOf() == ts.m_preliminary.graphOf());
    OGDF_ASSERT(subtree->graphOf() == ts.m_firstChild.graphOf());
    OGDF_ASSERT(subtree->graphOf() == ts.m_lastChild.graphOf());
    OGDF_ASSERT(subtree->graphOf() == ts.m_modifier.graphOf());
    OGDF_ASSERT(subtree->graphOf() == ts.m_change.graphOf());
    OGDF_ASSERT(subtree->graphOf() == ts.m_shift.graphOf());

    // compute a preliminary x-coordinate for subtree
    if(ts.isLeaf(subtree)) {

        // place subtree close to the left sibling
        ogdf::node leftSibling = ts.m_leftSibling[subtree];
        if(leftSibling != nullptr) {
            if(upDown) {
                ts.m_preliminary[subtree] = ts.m_preliminary[leftSibling]
                    + (ts.m_ga.width(subtree) + ts.m_ga.width(leftSibling)) / 2
                    + m_siblingDistance;
            } else {
                ts.m_preliminary[subtree] = ts.m_preliminary[leftSibling]
                    + (ts.m_ga.height(subtree) + ts.m_ga.height(leftSibling)) / 2
                    + m_siblingDistance;
            }
        }
        else ts.m_preliminary[subtree] = 0;
    }
    else {
        ogdf::node defaultAncestor = ts.m_firstChild[subtree];

        // collect the children of subtree
        ogdf::List<ogdf::node> children;
        ogdf::node v = ts.m_lastChild[subtree];
        do {
            children.pushFront(v);
            v = ts.m_leftSibling[v];
        } while(v != nullptr);

        ogdf::ListIterator<ogdf::node> it;

        // apply firstwalk and apportion to the children
        for (it = children.begin(); it.valid(); it = it.succ()) {
            firstWalk(ts, *it, upDown);
            apportion(ts, *it, defaultAncestor, upDown);
        }

        // shift the small subtrees
        double shift = 0;
        double change = 0;
        children.reverse();
        for(it = children.begin(); it.valid(); it = it.succ()) {
            ts.m_preliminary[*it] += shift;
            ts.m_modifier[*it] += shift;
            change += ts.m_change[*it];
            shift += ts.m_shift[*it] + change;
        }

        // place the parent node
        double midpoint = (ts.m_preliminary[children.front()] + ts.m_preliminary[children.back()]) / 2;
        ogdf::node leftSibling = ts.m_leftSibling[subtree];
        if(leftSibling != nullptr) {
            if(upDown) {
                ts.m_preliminary[subtree] = ts.m_preliminary[leftSibling]
                    + (ts.m_ga.width(subtree) + ts.m_ga.width(leftSibling)) / 2
                    + m_siblingDistance;
            } else {
                ts.m_preliminary[subtree] = ts.m_preliminary[leftSibling]
                    + (ts.m_ga.height(subtree) + ts.m_ga.height(leftSibling)) / 2
                    + m_siblingDistance;
            }
            ts.m_modifier[subtree] =
                ts.m_preliminary[subtree] - midpoint;
        }
        else ts.m_preliminary[subtree] = midpoint;
    }
}

void CustomOgdfTreeLayout::apportion(
    TreeStructure &ts,
    ogdf::node subtree,
    ogdf::node &defaultAncestor,
    bool upDown)
{
    OGDF_ASSERT(subtree != nullptr);
    OGDF_ASSERT(subtree->graphOf() == defaultAncestor->graphOf());
    OGDF_ASSERT(subtree->graphOf() == ts.m_leftSibling.graphOf());
    OGDF_ASSERT(subtree->graphOf() == ts.m_firstChild.graphOf());
    OGDF_ASSERT(subtree->graphOf() == ts.m_modifier.graphOf());
    OGDF_ASSERT(subtree->graphOf() == ts.m_ancestor.graphOf());
    OGDF_ASSERT(subtree->graphOf() == ts.m_change.graphOf());
    OGDF_ASSERT(subtree->graphOf() == ts.m_shift.graphOf());
    OGDF_ASSERT(subtree->graphOf() == ts.m_thread.graphOf());

    if(ts.m_leftSibling[subtree] == nullptr) return;

    // check distance to the left of the subtree
    // and traverse left/right inside/outside contour

    double leftModSumOut = 0;  // sum of modifiers on left outside contour
    double leftModSumIn = 0;   // sum of modifiers on left inside contour
    double rightModSumIn = 0;  // sum of modifiers on right inside contour
    double rightModSumOut = 0; // sum of modifiers on right outside contour

    double moveDistance;
    int numberOfSubtrees;
    ogdf::node leftAncestor,rightAncestor;

    // start the traversal at the actual level
    ogdf::node leftContourOut  = ts.m_firstChild[ts.m_parent[subtree]];
    ogdf::node leftContourIn   = ts.m_leftSibling[subtree];
    ogdf::node rightContourIn  = subtree;
    ogdf::node rightContourOut = subtree;
    bool stop = false;
    do {

        // add modifiers
        leftModSumOut  += ts.m_modifier[leftContourOut];
        leftModSumIn   += ts.m_modifier[leftContourIn];
        rightModSumIn  += ts.m_modifier[rightContourIn];
        rightModSumOut += ts.m_modifier[rightContourOut];

        // actualize ancestor for right contour
        ts.m_ancestor[rightContourOut] = subtree;

        if(ts.nextOnLeftContour(leftContourOut) != nullptr && ts.nextOnRightContour(rightContourOut) != nullptr)
        {
            // continue traversal
            leftContourOut  = ts.nextOnLeftContour(leftContourOut);
            leftContourIn   = ts.nextOnRightContour(leftContourIn);
            rightContourIn  = ts.nextOnLeftContour(rightContourIn);
            rightContourOut = ts.nextOnRightContour(rightContourOut);

            // check if subtree has to be moved
            if(upDown) {
                moveDistance = ts.m_preliminary[leftContourIn] + leftModSumIn
                    + (ts.m_ga.width(leftContourIn) + ts.m_ga.width(rightContourIn)) / 2
                    + m_subtreeDistance
                    - ts.m_preliminary[rightContourIn] - rightModSumIn;
            } else {
                moveDistance = ts.m_preliminary[leftContourIn] + leftModSumIn
                    + (ts.m_ga.height(leftContourIn) + ts.m_ga.height(rightContourIn)) / 2
                    + m_subtreeDistance
                    - ts.m_preliminary[rightContourIn] - rightModSumIn;
            }
            if(moveDistance > 0) {

                // compute highest different ancestors of leftContourIn
                // and rightContourIn
                if(ts.m_parent[ts.m_ancestor[leftContourIn]] == ts.m_parent[subtree])
                    leftAncestor = ts.m_ancestor[leftContourIn];
                else leftAncestor = defaultAncestor;
                rightAncestor = subtree;

                // compute the number of small subtrees in between (plus 1)
                numberOfSubtrees =
                    ts.m_number[rightAncestor] - ts.m_number[leftAncestor];

                // compute the shifts and changes of shift
                ts.m_change[rightAncestor] -= moveDistance / numberOfSubtrees;
                ts.m_shift[rightAncestor] += moveDistance;
                ts.m_change[leftAncestor] += moveDistance / numberOfSubtrees;

                // move subtree to the right by moveDistance
                ts.m_preliminary[rightAncestor] += moveDistance;
                ts.m_modifier[rightAncestor] += moveDistance;
                rightModSumIn += moveDistance;
                rightModSumOut += moveDistance;
            }
        }
        else stop = true;
    } while(!stop);

    // adjust threads
    if(ts.nextOnRightContour(rightContourOut) == nullptr && ts.nextOnRightContour(leftContourIn) != nullptr)
    {
        // right subtree smaller than left subforest
        ts.m_thread[rightContourOut] = ts.nextOnRightContour(leftContourIn);
        ts.m_modifier[rightContourOut] += leftModSumIn - rightModSumOut;
    }

    if(ts.nextOnLeftContour(leftContourOut) == nullptr && ts.nextOnLeftContour(rightContourIn) != nullptr)
    {
        // left subforest smaller than right subtree
        ts.m_thread[leftContourOut] = ts.nextOnLeftContour(rightContourIn);
        ts.m_modifier[leftContourOut] += rightModSumIn - leftModSumOut;
        defaultAncestor = subtree;
    }
}


void CustomOgdfTreeLayout::secondWalkX(
    TreeStructure &ts,
    ogdf::node subtree,
    double modifierSum)
{
    OGDF_ASSERT(subtree != nullptr);
    OGDF_ASSERT(subtree->graphOf() == ts.m_preliminary.graphOf());
    OGDF_ASSERT(subtree->graphOf() == ts.m_modifier.graphOf());

    // compute final x-coordinates for the subtree
    // by recursively aggregating modifiers
    ts.m_ga.x(subtree) = ts.m_preliminary[subtree] + modifierSum;
    modifierSum += ts.m_modifier[subtree];
    for(ogdf::adjEntry adj : subtree->adjEntries) {
        ogdf::edge e = adj->theEdge();
        if (e->target() != subtree)
            secondWalkX(ts, e->target(), modifierSum);
    }
}


void CustomOgdfTreeLayout::secondWalkY(
    TreeStructure &ts,
    ogdf::node subtree,
    double modifierSum)
{
    OGDF_ASSERT(subtree != nullptr);
    OGDF_ASSERT(subtree->graphOf() == ts.m_preliminary.graphOf());
    OGDF_ASSERT(subtree->graphOf() == ts.m_modifier.graphOf());

    // compute final y-coordinates for the subtree
    // by recursively aggregating modifiers
    ts.m_ga.y(subtree) = ts.m_preliminary[subtree] + modifierSum;
    modifierSum += ts.m_modifier[subtree];
    for(ogdf::adjEntry adj : subtree->adjEntries) {
        ogdf::node t = adj->theEdge()->target();
        if (t != subtree)
            secondWalkY(ts, t, modifierSum);
    }
}


void CustomOgdfTreeLayout::computeYCoordinatesAndEdgeShapes(ogdf::node root, ogdf::GraphAttributes &AG)
{
    OGDF_ASSERT(root != nullptr);

    // compute y-coordinates and edge shapes
    ogdf::List<ogdf::node> oldLevel;   // the nodes of the old level
    ogdf::List<ogdf::node> newLevel;   // the nodes of the new level
    ogdf::ListIterator<ogdf::node> it;
    double yCoordinate;    // the y-coordinate for the new level
    double edgeCoordinate; // the y-coordinate for edge bends
    double newHeight;      // the maximal node height on the new level

    // traverse the tree level by level
    newLevel.pushBack(root);
    AG.y(root) = yCoordinate = 0;
    newHeight = AG.height(root);
    while(!newLevel.empty()) {
        double oldHeight = newHeight; // the maximal node height on the old level
        newHeight = 0;
        oldLevel.conc(newLevel);
        while(!oldLevel.empty()) {
            ogdf::node v = oldLevel.popFrontRet();
            for(ogdf::adjEntry adj : v->adjEntries) {
                ogdf::edge e = adj->theEdge();
                if(e->target() != v) {
                    ogdf::node w = adj->theEdge()->target();
                    newLevel.pushBack(w);

                    // compute the shape of edge e
                    ogdf::DPolyline &edgeBends = AG.bends(e);
                    edgeBends.clear();
                    if(m_orthogonalLayout) {
                        edgeCoordinate =
                            yCoordinate + (oldHeight + m_levelDistance) / 2;
                        edgeBends.pushBack(ogdf::DPoint(AG.x(v),edgeCoordinate));
                        edgeBends.pushBack(ogdf::DPoint(AG.x(w),edgeCoordinate));
                    }

                    // compute the maximal node height on the new level
                    if(AG.height(e->target()) > newHeight)
                        newHeight = AG.height(e->target());
                }
            }
        }

        // assign y-coordinate to the nodes of the new level
        yCoordinate += (oldHeight + newHeight) / 2 + m_levelDistance;
        for(it = newLevel.begin(); it.valid(); it = it.succ())
            AG.y(*it) = yCoordinate;
    }
}

void CustomOgdfTreeLayout::computeXCoordinatesAndEdgeShapes(ogdf::node root, ogdf::GraphAttributes &AG)
{
    OGDF_ASSERT(root != nullptr);

    // compute y-coordinates and edge shapes
    ogdf::List<ogdf::node> oldLevel;   // the nodes of the old level
    ogdf::List<ogdf::node> newLevel;   // the nodes of the new level
    ogdf::ListIterator<ogdf::node> it;
    double xCoordinate;    // the x-coordinate for the new level
    double edgeCoordinate; // the x-coordinate for edge bends
    double newWidth;       // the maximal node width on the new level

    // traverse the tree level by level
    newLevel.pushBack(root);
    AG.x(root) = xCoordinate = 0;
    newWidth = AG.width(root);
    while(!newLevel.empty()) {
        double oldWidth = newWidth; // the maximal node width on the old level
        newWidth = 0;
        oldLevel.conc(newLevel);
        while(!oldLevel.empty()) {
            ogdf::node v = oldLevel.popFrontRet();
            for(ogdf::adjEntry adj : v->adjEntries) {
                ogdf::edge e = adj->theEdge();
                if(e->target() != v) {
                    ogdf::node w = e->target();
                    newLevel.pushBack(w);

                    // compute the shape of edge e
                    ogdf::DPolyline &edgeBends = AG.bends(e);
                    edgeBends.clear();
                    if(m_orthogonalLayout) {
                        edgeCoordinate =
                            xCoordinate + (oldWidth + m_levelDistance) / 2;
                        edgeBends.pushBack(ogdf::DPoint(edgeCoordinate,AG.y(v)));
                        edgeBends.pushBack(ogdf::DPoint(edgeCoordinate,AG.y(w)));
                    }

                    // compute the maximal node width on the new level
                    if(AG.width(e->target()) > newWidth)
                        newWidth = AG.width(e->target());
                }
            }
        }

        // assign x-coordinate to the nodes of the new level
        xCoordinate += (oldWidth + newWidth) / 2 + m_levelDistance;
        for(it = newLevel.begin(); it.valid(); it = it.succ())
            AG.x(*it) = xCoordinate;
    }
}
