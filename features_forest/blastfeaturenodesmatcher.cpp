#include "blastfeaturenodesmatcher.h"

#include "graphsearch/blast/blastsearch.h"
#include "graphsearch/hit.h"
#include "graphsearch/queries.h"
#include "graphsearch/query.h"
#include "program/globals.h"
#include "program/settings.h"

BlastFeaturesNodesMatcher::BlastFeaturesNodesMatcher() { }

void BlastFeaturesNodesMatcher::matchFeaturesNode(GraphSearchDialog* graphSearchDialog, QList<FeatureTreeNode*> selectedNodes, search::GraphSearch* graphSearch) {
    bool isQueriesAdded = false;
    for(FeatureTreeNode* selectedNode : selectedNodes) {
        std::vector<QString> querySequences = selectedNode->getQuerySequences();
        if ((querySequences.size()!= 0) && (selectedNode->getBlastColourInd() == - 1)) {
            QString featureNodeName = selectedNode->getName() + "_";
            size_t indexColour = graphSearch->getQueryCount();
            for (size_t i = 0; i < querySequences.size(); ++i)
            {
                QString queryName = featureNodeName + QString::number(i);
                graphSearch->addQuery(new search::Query(queryName, querySequences[i]), indexColour, selectedNode->getClassInd());
            }
            isQueriesAdded = true;
            selectedNode->setBlastColourInd(indexColour);
        }
    }
    if (isQueriesAdded) {
        if (!(graphSearchDialog -> isDbBuild())) {
            graphSearchDialog->buildDatabase(false, false);
        }
        graphSearchDialog->runGraphSearches(false, QString("-word_size 31 -dust no"), false);
    }

}
