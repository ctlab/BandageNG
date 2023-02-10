//Copyright 2017 Ryan Wick

//This file is part of Bandage

//Bandage is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

//Bandage is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.

//You should have received a copy of the GNU General Public License
//along with Bandage.  If not, see <http://www.gnu.org/licenses/>.


#include "treelayoutworker.h"
#include <time.h>
#include "ogdf/basic/GraphAttributes.h"
#include "ogdf/basic/geometry.h"
#include "ogdf/tree/TreeLayout.h"
#include <QLineF>
#include <QtConcurrent>
#include "../program/settings.h"
#include "../program/globals.h"
#include "../features_forest/featuretreenode.h"
#include "../features_forest/assemblyfeaturesforest.h"
#include "../program/globals.h"
#include "layout/customogdftreelayout.h"


TreeLayouter::TreeLayouter() {}

class TreeGraphLayout : public TreeLayouter {
public:

    void init() override {
        init(m_treeLayout);
    }

    void cancel() override {
    }

    void run(ogdf::GraphAttributes &GA) override {
        m_treeLayout.callMultiLine(GA);
    }

private:
    void init(CustomOgdfTreeLayout &layout) const {
        layout.orientation(ogdf::Orientation::topToBottom);
        layout.subtreeDistance(50.0);
        layout.siblingDistance(50.0);
    }

    CustomOgdfTreeLayout m_treeLayout;
};

TreeLayoutWorker::TreeLayoutWorker() { }

using OGDFGraphLayout = FeaturesLayoutStorage<ogdf::node>;

static void addEdgeToOgdfGraph(FeatureTreeNode* startingNode,
                               FeatureTreeNode* endingNode,
                               ogdf::Graph &ogdfGraph,
                               ogdf::EdgeArray<double> &edgeArray,
                               const OGDFGraphLayout &layout) {
    ogdf::node firstEdgeOgdfNode;
    ogdf::node secondEdgeOgdfNode;

    if (layout.contains(startingNode))
        firstEdgeOgdfNode = layout.segments(startingNode).front();
    else
        return;

    if (layout.contains(endingNode))
        secondEdgeOgdfNode = layout.segments(endingNode).back();
    else
        return;
    ogdf::edge newEdge = ogdfGraph.newEdge(firstEdgeOgdfNode, secondEdgeOgdfNode);
    edgeArray[newEdge] = g_settings->featureForestEdgeLength;
}

static void buildOgdfGraphFromNodesAndEdges(ogdf::Graph &ogdfGraph,
                                            ogdf::GraphAttributes &ogdfGraphAttributes,
                                            ogdf::EdgeArray<double> &ogdfEdgeLengths,
                                            OGDFGraphLayout &layout)
{
    const AssemblyFeaturesForest &graph = layout.graph();
    for (auto *node : graph.m_nodes) {
        ogdf::node newNode = ogdfGraph.newNode();
        layout.add(node, newNode);
    }


    QMapIterator<QString, FeatureTreeNode*> j(graph.m_nodes);
    while (j.hasNext())
    {
        j.next();
        FeatureTreeNode* node = j.value();
            for (FeatureTreeNode* child : node->getChildren()) {
                addEdgeToOgdfGraph(node, child, ogdfGraph, ogdfEdgeLengths, layout);
            }
    }
}

FeaturesLayout TreeLayoutWorker::layoutGraph(const AssemblyFeaturesForest &graph) {
    ogdf::Graph G;
    ogdf::EdgeArray<double> edgeLengths(G);
    ogdf::GraphAttributes GA(G,
                             ogdf::GraphAttributes::nodeGraphics | ogdf::GraphAttributes::edgeGraphics);
    OGDFGraphLayout layout(graph);
    buildOgdfGraphFromNodesAndEdges(G, GA, edgeLengths, layout);

    auto treeLayouter = new TreeGraphLayout();
    m_taskSynchronizer.addFuture(
            QtConcurrent::run([&](TreeLayouter *treeLayouter) {
                treeLayouter->run(GA);
            }, treeLayouter));
    m_taskSynchronizer.waitForFinished();

    FeaturesLayout res(graph);
    for (const auto & entry : layout) {
        for (ogdf::node node : entry.second) {
            res.add(entry.first, { GA.x(node), GA.y(node) });
        }
    }

    return res;
}

void TreeLayoutWorker::cancelLayout() {
    for (auto & future : m_taskSynchronizer.futures())
        future.cancel();
}
