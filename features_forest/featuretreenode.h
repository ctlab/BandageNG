#ifndef FeatureTreeNode_H
#define FeatureTreeNode_H

#include <QString>
#include <QColor>
#include <vector>
#include "../program/globals.h"
#include "../program/settings.h"

class GraphicsItemFeatureNode;
class FeatureTreeEdge;

class FeatureTreeNode
{
public:
    FeatureTreeNode(QString name);
    ~FeatureTreeNode() = default;

    void cleanUp();
    void addChild(FeatureTreeNode * child);
    void setDepth(int depth) { m_depth = depth; }

    int getDepth() { return m_depth; }
    QString getName() { return m_name; }

    FeatureTreeNode* getParent() { return m_parent; }
    void setParent(FeatureTreeNode* parent) { m_parent = parent; }

    GraphicsItemFeatureNode * getGraphicsItemFeatureNode() { return m_graphicsItemFeatureNode; }
    void setGraphicsItemFeatureNode(GraphicsItemFeatureNode* graphicsItemNode) { m_graphicsItemFeatureNode = graphicsItemNode; }
    bool hasGraphicsItemFeature() { return m_graphicsItemFeatureNode != NULL; }

    std::vector<FeatureTreeNode*> getChildren() { return m_children; }

    std::vector<QString> getQuerySequences();
    void setFeature(QString featureName, double threshold) { m_featureName = featureName; m_threshold = threshold; }
    QString getFeatureName() { return m_featureName; }
    double getThreshold() { return m_threshold; }
    void setClass(int classInd, QString clazz) { m_classInd = classInd; m_class = clazz; }
    QString getClass() { return m_class; }
    int getClassInd() { return m_classInd; }
    int getBlastColourInd() { return m_blastColourIndex; }
    void setBlastColourInd(int blastColourInd) { m_blastColourIndex = blastColourInd; }
    bool hasCustomColour() { return m_customColour.isValid(); }
    QColor getCustomColour();
    void setCustomColour(QColor colour) { m_customColour = colour; }
    QString getCustomLabel() const { return m_customLabel; }
    QStringList getCustomLabelForDisplay() const;
    void setCustomLabel(QString newLabel);
    std::vector<FeatureTreeEdge*>* getEdges() { return &m_edges; }
    void addEdge(FeatureTreeEdge* edge) { m_edges.push_back(edge); }
    QByteArray getFasta(bool newLines, bool evenIfEmpty);
    QByteArray getNodeNameForFasta(int currentSeqInd);
    //bool sequenceIsMissing() { return m_querySequences.empty(); }

private:
    QString m_name;
    int m_depth = 0;
    std::vector<FeatureTreeNode*> m_children;
    FeatureTreeNode * m_parent = NULL;
    //std::vector<QString> m_querySequences;

    GraphicsItemFeatureNode * m_graphicsItemFeatureNode;
    QString m_featureName;
    double m_threshold = 0;
    QString m_class;
    int m_classInd = -1;
    int m_blastColourIndex = -1;
    QColor m_customColour;
    QString m_customLabel;

    std::vector<FeatureTreeEdge*> m_edges;

};

#endif // FeatureTreeNode_H

