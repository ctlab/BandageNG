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

#pragma once

#include <QObject>
#include <vector>

#include <QString>
#include <QMap>
#include "../program/globals.h"
#include "../ui/bandagegraphicsscene.h"
#include <QPair>
#include "featuretreeedge.h"
#include "featuretreenode.h"
#include "../program/globals.h"

#include "parallel_hashmap/phmap.h"

class FeatureTreeNode;
class FeatureTreeEdge;
class MyProgressDialog;

class AssemblyFeaturesForest : public QObject
{
    Q_OBJECT

public:
    AssemblyFeaturesForest();
    ~AssemblyFeaturesForest();
    bool loadFeatureForestFromFile(QString filename, QString* errormsg);

    QMap<QString, FeatureTreeNode*> m_roots;
    QMap<QString, FeatureTreeNode*> m_nodes;
    QMap<QPair<FeatureTreeNode*, FeatureTreeNode*>, FeatureTreeEdge*> m_edges;
    QMap<QString, int> m_classes;
    phmap::parallel_flat_hash_map<const FeatureTreeNode*, QColor> m_nodeColors;

    void cleanUp();
    void recalculateAllNodeWidths();
    void addClass(QString className);
    void resetNodes();
    void resetEdges();
    QString getClassFigureInfo();
    QColor getCustomColourForDisplay(const FeatureTreeNode *node) const;

    bool hasCustomColour(const FeatureTreeNode* node) const;
    QColor getCustomColour(const FeatureTreeNode* node) const;
    void setCustomColour(const FeatureTreeNode* node, QColor color);
    void addSequence(QString feature, QString seq) {
        m_featuresSeq[feature].push_back(seq);
    }
    std::vector<QString> getSequences(QString feature) {
        return m_featuresSeq[feature];
    }
    bool isSequenceMissing(QString feature) { return m_featuresSeq.contains(feature); }
private:
    phmap::parallel_flat_hash_map<QString, std::vector<QString>> m_featuresSeq;

    void processChild(FeatureTreeNode* parent, QString childName);
    void addEdgeToOgdfGraph(FeatureTreeNode* startNode, FeatureTreeNode* lastNode);


};
