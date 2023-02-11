// Copyright 2022 Anton Korobeynikov

// This file is part of Bandage-NG

// Bandage is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// Bandage is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with Bandage.  If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include <QColor>
#include <QSharedPointer>

class AssemblyFeaturesForest;
class GraphicsItemFeatureNode;

// This needs to be synchronizes with selection combo box!
enum FeatureNodeColorScheme : int {
    FEATURE_UNIFORM_COLOURS = 0,
    FEATURE_CLASS_COLOURS = 1,
    FEATURE_CUSTOM_COLOURS = 2,
    FEATURE_BLAST_SOLID_COLOURS = 3,
    FEATURE_BLAST_CLASS_COLOURS = 4,
    FEATURE_LAST_SCHEME = FEATURE_BLAST_CLASS_COLOURS
};

class IFeatureNodeColorer {
public:
    explicit IFeatureNodeColorer(FeatureNodeColorScheme scheme);
    virtual ~IFeatureNodeColorer() = default;

    [[nodiscard]] virtual QColor get(const GraphicsItemFeatureNode *node) = 0;
    virtual void reset() {};
    [[nodiscard]] virtual const char* name() const = 0;

    static std::unique_ptr<IFeatureNodeColorer> create(FeatureNodeColorScheme scheme);

    [[nodiscard]] FeatureNodeColorScheme scheme() const { return m_scheme; }
protected:
    FeatureNodeColorScheme m_scheme;
    QSharedPointer<AssemblyFeaturesForest> m_graph;
};
