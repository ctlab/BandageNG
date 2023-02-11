// Copyright 2022 Anton Korobeynikov

// This file is part of Bandage

// Bandage is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

// Bandage is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with Bandage.  If not, see <http://www.gnu.org/licenses/>.

#include "featureslayout.h"

#include "features_forest/assemblyfeaturesforest.h"
#include "features_forest/featuretreenode.h"
#include "features_forest/graphicsitemfeaturenode.h"

namespace layout {
    FeaturesLayout fromGraph(const AssemblyFeaturesForest &graph) {
        FeaturesLayout res(graph);

        for (auto *node : graph.m_nodes) {
            if (!node->hasGraphicsItemFeature())
                continue;

            const auto &segments = node->getGraphicsItemFeatureNode()->m_linePoints;
            res.segments(node) = segments;

        }

        return res;
    }

    void apply(AssemblyFeaturesForest &graph, const FeaturesLayout &layout) {
        graph.resetNodes();
    }
}
