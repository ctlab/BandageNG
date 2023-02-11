#include "featuretreenode.h"
#include "graph/sequenceutils.h"
#include "features_forest/assemblyfeaturesforest.h"

FeatureTreeNode::FeatureTreeNode(QString name) :
	m_name(name)
{}

void FeatureTreeNode::cleanUp() {
    m_children.clear();
    m_edges.clear();
}

void FeatureTreeNode::addChild(FeatureTreeNode * child) {
	m_children.push_back(child);
    child->setDepth(m_depth + 1);
	child->setParent(this);
}

QColor FeatureTreeNode::getCustomColour() {
	if (hasCustomColour()) {
		return m_customColour;
	}
	else {
		return g_settings->defaultCustomNodeColour;
	}
}

QStringList FeatureTreeNode::getCustomLabelForDisplay() const
{
    QStringList customLabelLines;
    if (!getCustomLabel().isEmpty()) {
        QStringList labelLines = getCustomLabel().split("\\n");
        for (int i = 0; i < labelLines.size(); ++i)
            customLabelLines << labelLines[i];
    }
    return customLabelLines;
}

void FeatureTreeNode::setCustomLabel(QString newLabel)
{
    newLabel.replace("\t", "    ");
    m_customLabel = newLabel;
}

QByteArray FeatureTreeNode::getFasta(bool newLines, bool evenIfEmpty) {
    int currentSeqInd = 0;
    QByteArray fasta = "";
    for(QString seq : this->getQuerySequences()) {
        QByteArray sequence = seq.toUtf8();
        if (sequence.isEmpty() && !evenIfEmpty)
            continue;

        fasta += ">";
        currentSeqInd += 1;
        fasta += getNodeNameForFasta(currentSeqInd);
        fasta += "\n";
        if (newLines)
            fasta += utils::addNewlinesToSequence(sequence);
        else {
            fasta += sequence;
            fasta += "\n";
        }
    }
    return fasta;
}

QByteArray FeatureTreeNode::getNodeNameForFasta(int currentSeqInd)
{
    QByteArray nodeNameForFasta;

    nodeNameForFasta += "NODE_";
    nodeNameForFasta += qPrintable(this->getName());
    nodeNameForFasta += "_";
    nodeNameForFasta += QByteArray::number(currentSeqInd);

    return nodeNameForFasta;
}

std::vector<QString> FeatureTreeNode::getQuerySequences() { return g_assemblyFeaturesForest->getSequences(getFeatureName()); }
