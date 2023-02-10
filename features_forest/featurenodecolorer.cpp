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

#include "featurenodecolorer.h"
#include "featurenodecolorers.h"

#include "assemblyfeaturesforest.h"
#include "featuretreenode.h"
#include "graphicsitemfeaturenode.h"

#include "program/colormap.h"
#include "program/globals.h"
#include "program/settings.h"

#define TINYCOLORMAP_WITH_QT5
#include <colormap/tinycolormap.hpp>
#include "parallel_hashmap/phmap.h"

#include <unordered_set>

#include <QApplication>

IFeatureNodeColorer::IFeatureNodeColorer(FeatureNodeColorScheme scheme)
    : m_graph(g_assemblyFeaturesForest), m_scheme(scheme) {
}

std::unique_ptr<IFeatureNodeColorer> IFeatureNodeColorer::create(FeatureNodeColorScheme scheme) {
    switch (scheme) {
        case FEATURE_UNIFORM_COLOURS:
            return std::make_unique<FeatureUniformNodeColorer>(scheme);
        case FEATURE_CLASS_COLOURS:
            return std::make_unique<FeatureClassNodeColorer>(scheme);
        case FEATURE_CUSTOM_COLOURS:
            return std::make_unique<FeatureCustomNodeColorer>(scheme);
        case FEATURE_BLAST_SOLID_COLOURS:
            return std::make_unique<FeatureBlastSolidNodeColorer>(scheme);
        case FEATURE_BLAST_CLASS_COLOURS:
            return std::make_unique<FeatureBlastClassNodeColorer>(scheme);
    }

    return nullptr;
}

QColor FeatureUniformNodeColorer::get(const GraphicsItemFeatureNode *node) {
    return g_settings->uniformPositiveNodeColour;
}

QColor FeatureCustomNodeColorer::get(const GraphicsItemFeatureNode *node) {
    return m_graph->getCustomColourForDisplay(node->m_featureNode);
}

QColor FeatureClassNodeColorer::get(const GraphicsItemFeatureNode *node) {
    int classInd = (node->m_featureNode->getClassInd()) % 200;
    return getPresetColours()[classInd];
}

QColor FeatureBlastSolidNodeColorer::get(const GraphicsItemFeatureNode *node) {
    int blastInd = node->m_featureNode->getBlastColourInd();
    if (blastInd == -1) {
        return g_settings->grayColor;
    }
    blastInd %= 200;
    return getPresetColours()[blastInd];
}

QColor FeatureBlastClassNodeColorer::get(const GraphicsItemFeatureNode *node) {
    int blastInd = node->m_featureNode->getBlastColourInd();
    if (blastInd == -1) {
        return g_settings->grayColor;
    }
    int classInd = (node->m_featureNode->getClassInd()) % 200;
    return getPresetColours()[classInd];
}

