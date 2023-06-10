#ifndef BLASTFEATURENODESMATCHER_H
#define BLASTFEATURENODESMATCHER_H

#include <QList>
#include "features_forest/featuretreenode.h"
#include "graphsearch/graphsearch.h"
#include "ui/dialogs/graphsearchdialog.h"

class BlastFeaturesNodesMatcher
{
public:
    BlastFeaturesNodesMatcher();
    void matchFeaturesNode(GraphSearchDialog* graphSearchDialog, QList<FeatureTreeNode*> selectedNodes, search::GraphSearch* graphSearch);
private:
};

#endif // BLASTFEATURENODESMATCHER_H
