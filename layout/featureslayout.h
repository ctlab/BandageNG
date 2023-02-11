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

#pragma once

#include "parallel_hashmap/phmap.h"
#include "small_vector/small_pod_vector.hpp"

#include <QPointF>

class FeatureTreeNode;
class AssemblyFeaturesForest;

template<class T>
class FeaturesLayoutStorage {
public:
    explicit FeaturesLayoutStorage(const AssemblyFeaturesForest &graph)
            : m_graph(graph) {}

    [[nodiscard]] const AssemblyFeaturesForest &graph() const { return m_graph; }
    bool contains(const FeatureTreeNode *node) const { return m_data.contains(node); }
    void add(FeatureTreeNode *node, T point) { m_data[node].emplace_back(point); }
    size_t size() const { return m_data.size(); }

    const auto& segments(const FeatureTreeNode *node) const { return m_data.at(node); }
    auto& segments(FeatureTreeNode *node) { return m_data[node]; }

    auto begin() { return m_data.begin(); }
    auto begin() const { return m_data.begin(); }
    auto end() { return m_data.end(); }
    auto end() const { return m_data.end(); }

private:
    const AssemblyFeaturesForest &m_graph;
    phmap::flat_hash_map<FeatureTreeNode*, adt::SmallPODVector<T>> m_data;
};

using FeaturesLayout = FeaturesLayoutStorage<QPointF>;

namespace layout {
    FeaturesLayout fromGraph(const AssemblyFeaturesForest &graph);
    void apply(AssemblyFeaturesForest &graph, const FeaturesLayout &layout);
}
