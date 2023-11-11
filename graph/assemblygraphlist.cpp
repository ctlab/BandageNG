#include "assemblygraphlist.h"
#include "program/globals.h"
#include "program/settings.h"

AssemblyGraphList::AssemblyGraphList()
{
}

QColor AssemblyGraphList::getCustomColourForDisplay(const DeBruijnNode *node) const {
    if (node->m_customColor.isValid())
        return node->m_customColor;

    DeBruijnNode *rc = node->getReverseComplement();
    if (!g_settings->doubleMode && rc->m_customColor.isValid())
        return rc->m_customColor;
    return g_settings->defaultCustomNodeColour;
}

QStringList AssemblyGraphList::getCustomLabelForDisplay(const DeBruijnNode *node) const {
    QStringList customLabelLines;
    QString label = node->m_customLabel;
    if (!label.isEmpty()) {
        QStringList labelLines = label.split("\n");
        for (auto & labelLine : labelLines)
            customLabelLines << labelLine;
    }

    DeBruijnNode *rc = node->getReverseComplement();
    if (!g_settings->doubleMode && !rc->m_customLabel.isEmpty()) {
        QStringList labelLines2 = rc->m_customLabel.split("\n");
        for (auto & i : labelLines2)
            customLabelLines << i;
    }
    return customLabelLines;
}


static QStringList removeNullStringsFromList(const QStringList& in) {
    QStringList out;

    for (const auto& string : in) {
        if (string.length() > 0)
            out.push_back(string);
    }
    return out;
}

bool AssemblyGraphList::checkIfStringHasNodes(QString nodesString) {
    nodesString = nodesString.simplified();
    QStringList nodesList = nodesString.split(",");
    nodesList = removeNullStringsFromList(nodesList);
    return (nodesList.empty());
}

QString AssemblyGraphList::generateNodesNotFoundErrorMessage(std::vector<QString> nodesNotInGraph, bool exact) {
    QString errorMessage;
    if (exact)
        errorMessage += "The following nodes are not in the graph:\n";
    else
        errorMessage += "The following queries do not match any nodes in the graph:\n";

    for (size_t i = 0; i < nodesNotInGraph.size(); ++i) {
        errorMessage += nodesNotInGraph[i];
        if (i != nodesNotInGraph.size() - 1)
            errorMessage += ", ";
    }
    errorMessage += "\n";

    return errorMessage;
}

