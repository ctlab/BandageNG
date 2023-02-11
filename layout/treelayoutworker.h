#ifndef TREELAYOUTWORKER_H
#define TREELAYOUTWORKER_H

#include <QObject>
#include <QFutureSynchronizer>
#include "featureslayout.h"

namespace ogdf {
    class Graph;
    class GraphAttributes;
    template<class T> class EdgeArray;
}

class AssemblyFeaturesForest;

class TreeLayouter {
public:
    TreeLayouter();
    virtual ~TreeLayouter() {}
    virtual void init() = 0;
    virtual void cancel() = 0;
    virtual void run(ogdf::GraphAttributes &GA) = 0;

protected:
};

class TreeLayoutWorker : public QObject
{
    Q_OBJECT

public:
    TreeLayoutWorker();
    ~TreeLayoutWorker() override = default;

    FeaturesLayout layoutGraph(const AssemblyFeaturesForest &graph);
private:
    QFutureSynchronizer<void> m_taskSynchronizer;
public slots:
    //FeaturesLayout layoutGraph(const AssemblyFeaturesForest &graph);
    [[maybe_unused]] void cancelLayout();
};
#endif // TREELAYOUTWORKER_H
