#include "assemblyfeaturesforest.h"
#include <QFile>
#include <QTextStream>
#include <QApplication>
#include "../program/globals.h"
#include "graphicsitemfeaturenode.h"
#include "graphicsitemfeatureedge.h"

AssemblyFeaturesForest::AssemblyFeaturesForest() {}

AssemblyFeaturesForest::~AssemblyFeaturesForest() {}

bool AssemblyFeaturesForest::loadFeatureForestFromFile(QString filename, QString* errormsg) {
    QFile featureForestFile(filename);

    if (!featureForestFile.open(QIODevice::ReadOnly))
    {
        *errormsg = "Unable to read from specified file.";
        return false;
    }

    QTextStream in(&featureForestFile);
    QApplication::processEvents();
    while (!in.atEnd())
    {
        QString line = in.readLine();
        QStringList data = line.split("\t");
        if (data.size() <= 1) {
            break;
        }
        if (data[0] == "N") {
            QString nodeName = data[1];
            FeatureTreeNode* curnode = m_nodes[nodeName];
            if (curnode == nullptr) {
                curnode = new FeatureTreeNode(nodeName);
                m_nodes.insert(nodeName, curnode);
            }
            if (data.size() > 2 && data[2].size() > 0) {
                processChild(curnode, data[2]);
            }
            if (data.size() > 3 && data[3].size() > 0) {
                processChild(curnode, data[3]);
            }

        }
        if (data[0] == "S") {
            QString featureName = data[1];
            QString seq = data[2].simplified();
            g_assemblyFeaturesForest->addSequence(featureName, seq);
        }
        if (data[0] == "F") {
            QString nodeName = data[1];
            QString featureName = data[2];
            double threshold = data[3].toDouble();
            m_nodes[nodeName]->setFeature(featureName, threshold);
        }
        if (data[0] == "C") {
            QString nodeName = data[1];
            QString clazz = data[2].simplified();
            addClass(clazz);
            m_nodes[nodeName]->setClass(m_classes[clazz], clazz);
        }

    }
    featureForestFile.close();
    for (FeatureTreeNode* node : m_nodes) {
        if (node->getParent() == NULL) {
            m_roots[node->getName()] = node;
        }
    }
    return true;
}

void AssemblyFeaturesForest::recalculateAllNodeWidths()
{
    QMapIterator<QString, FeatureTreeNode*> i(m_nodes);
    while (i.hasNext())
    {
        i.next();
        GraphicsItemFeatureNode* graphicsItemNode = i.value()->getGraphicsItemFeatureNode();
        if (graphicsItemNode != 0)
            graphicsItemNode->m_width = g_settings->averageFeatureNodeWidth;
    }
}

void AssemblyFeaturesForest::cleanUp() {
    {
        for (auto &entry : m_nodes) {
            entry->cleanUp();
            delete entry;
        }
        m_nodes.clear();
    }

    {
        for (auto &entry : m_edges) {
            delete entry;
        }
        m_edges.clear();
    }
    m_roots.clear();
    m_classes.clear();
    m_nodeColors.clear();
    m_featuresSeq.clear();
}


void AssemblyFeaturesForest::processChild(FeatureTreeNode* parent, QString childName) {
    if (m_nodes.contains(childName)) {
        FeatureTreeNode* child = m_nodes[childName];
        parent->addChild(child);
        FeatureTreeEdge* edge = new FeatureTreeEdge(parent, child);
        parent->addEdge(edge);
        child->addEdge(edge);
        m_edges.insert(QPair<FeatureTreeNode*, FeatureTreeNode*>(parent, child), edge);

    }
    else {
        FeatureTreeNode* child = new FeatureTreeNode(childName);
        m_nodes.insert(childName, child);
        parent->addChild(child);
        FeatureTreeEdge* edge = new FeatureTreeEdge(parent, child);
        parent->addEdge(edge);
        child->addEdge(edge);
        m_edges.insert(QPair<FeatureTreeNode*, FeatureTreeNode*>(parent, child), edge);
    }
   
}

void AssemblyFeaturesForest::resetNodes()
{
    QMapIterator<QString, FeatureTreeNode*> i(m_nodes);
    while (i.hasNext())
    {
        i.next();
        i.value()->setGraphicsItemFeatureNode(nullptr);
    }
}

void AssemblyFeaturesForest::resetEdges()
{
    for (auto &entry : m_edges) {
        entry->setGraphicsItemFeatureEdge(nullptr);
    }
}


void AssemblyFeaturesForest::addClass(QString className) {
    int ind = m_classes.size();
    if (!m_classes.contains(className)) {
        m_classes[className] = ind;
    }
}

QString AssemblyFeaturesForest::getClassFigureInfo() {
    QString res = "";
    for (QString className : m_classes.keys()) {
        res = res + className + ": ";
        if (m_classes[className] == 0) {
            res += QChar(0x2B1B);
            res += " (cube)\n";
        } 
        else if (m_classes[className] == 1) {
            res += QChar(0x25B2);
            res += " (triangle)\n";
        }
        else {
            res += QChar(0x2B24);
            res += " (circle)\n";
        }
    }
    return res;
}

QColor AssemblyFeaturesForest::getCustomColourForDisplay(const FeatureTreeNode *node) const {
    if (hasCustomColour(node))
        return getCustomColour(node);
    return g_settings->defaultCustomNodeColour;
}

bool AssemblyFeaturesForest::hasCustomColour(const FeatureTreeNode* node) const {
    auto it = m_nodeColors.find(node);
    return it != m_nodeColors.end() && it->second.isValid();
}

QColor AssemblyFeaturesForest::getCustomColour(const FeatureTreeNode* node) const {
    auto it = m_nodeColors.find(node);
    return it == m_nodeColors.end() ? QColor() : it->second;
}

void AssemblyFeaturesForest::setCustomColour(const FeatureTreeNode* node, QColor color) {
    m_nodeColors[node] = color;
}
