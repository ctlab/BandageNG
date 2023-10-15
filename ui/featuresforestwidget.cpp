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

#include "featuresforestwidget.h"
#include <QFileDialog>
#include <QLatin1String>
#include <QTextStream>
#include <QLocale>
#include <math.h>
#include "../program/settings.h"
#include <QClipboard>
#include <QTransform>
#include <QFontDialog>
#include <QColorDialog>
#include <algorithm>
#include <QFile>
#include <QTextStream>
#include <QScrollBar>
#include <QFutureWatcher>
#include <QtConcurrent>
#include "../ui/dialogs/settingsdialog.h"
#include <stdlib.h>
#include <time.h>
#include <QProgressDialog>
#include <QThread>
#include <QMessageBox>
#include <QInputDialog>
#include <QShortcut>
#include "../ui/dialogs/aboutdialog.h"
#include "../graph/assemblygraph.h"
#include "bandagegraphicsview.h"
#include "graphicsviewzoom.h"
#include "bandagegraphicsscene.h"
#include "../graph/debruijnnode.h"
#include "../graph/debruijnedge.h"
#include "../graph/graphicsitemnode.h"
#include "../graph/graphicsitemedgecommon.h"
#include "../ui/dialogs/myprogressdialog.h"
#include <limits>
#include <QDesktopServices>
#include <QSvgGenerator>
#include "../graph/path.h"
#include "../ui/dialogs/pathspecifydialog.h"
#include "../program/memory.h"
#include "../ui/dialogs/changenodenamedialog.h"
#include "../ui/dialogs/changenodedepthdialog.h"
#include <limits>
#include "../ui/dialogs/graphinfodialog.h"
#include<iostream>
#include "../features_forest/assemblyfeaturesforest.h"
#include "../features_forest/featuretreenode.h"
#include "../features_forest/graphicsitemfeaturenode.h"
#include "../layout/treelayoutworker.h"

FeaturesForestWidget::FeaturesForestWidget(MainWindow *parent) : QObject(parent)
{
    mainWindow = parent;
    g_absoluteZoomFeatures = 1.0;
    m_graphicsViewZoom = new GraphicsViewZoom(g_graphicsViewFeaturesForest, &g_absoluteZoomFeatures);
    g_graphicsViewFeaturesForest->m_zoom = m_graphicsViewZoom;
    m_scene = new BandageGraphicsScene(this);
    g_graphicsViewFeaturesForest->setScene(m_scene);
    connect(m_graphicsViewZoom, SIGNAL(zoomed()), mainWindow, SLOT(zoomedFeaturesWithMouseWheel()));
}

FeaturesForestWidget::~FeaturesForestWidget()
{
}

void FeaturesForestWidget::cleanUp() {
    m_scene->blockSignals(true);
    g_graphicsViewFeaturesForest->setScene(nullptr);
    delete m_scene;
}

void FeaturesForestWidget::getSelectedNodeInfo(int & selectedNodeCount, QString & selectedFeatureNodeText)
{
    std::vector<FeatureTreeNode *> selectedNodes = m_scene->getSelectedFeatureNodes();

    selectedNodeCount = int(selectedNodes.size());

    for (int i = 0; i < selectedNodeCount; ++i)
    {
        QString nodeName = selectedNodes[i]->getName();
        selectedFeatureNodeText += "Id: ";
        selectedFeatureNodeText += nodeName;
        selectedFeatureNodeText += '\n';
        if (selectedNodes[i]->getFeatureName() != nullptr) {
            QString threshold = QString::number(selectedNodes[i]->getThreshold(), 'g', 2);
            selectedFeatureNodeText += "Rule: Feature " + selectedNodes[i]->getFeatureName() + " <= " + threshold + "\n";
        }
        if (selectedNodes[i]->getClass() != nullptr) {
            selectedFeatureNodeText += "Class: " + selectedNodes[i]->getClass() + "\n";
        }
        if (selectedNodes[i]->getDetails() != nullptr) {
            selectedFeatureNodeText += "Details:\n" + selectedNodes[i]->getDetails() + "\n";
        }
        selectedFeatureNodeText += "Sequence(s):\n";
        for (QString seq : selectedNodes[i]->getQuerySequences()) {
            selectedFeatureNodeText += "Seq: ";
            selectedFeatureNodeText += seq;
            selectedFeatureNodeText += "\n";

        }
    }
}

void FeaturesForestWidget::drawGraph(QWidget * parent) {
    resetScene();
    g_assemblyFeaturesForest->resetNodes();
    layoutGraph();
}

void FeaturesForestWidget::graphLayoutFinished(const FeaturesLayout &layout) {
    m_scene->clear();
    m_scene->addGraphicsItemsToScene(*g_assemblyFeaturesForest, layout);
    m_scene->setSceneRectangle();

    g_graphicsViewFeaturesForest->viewport()->update();
    g_graphicsViewFeaturesForest->setFocus();
    m_scene->selectionChanged();
}

void FeaturesForestWidget::graphLayoutCancelled()
{
}

void FeaturesForestWidget::resetScene()
{
    m_scene->blockSignals(true);

    g_graphicsViewFeaturesForest->setScene(0);
    delete m_scene;
    m_scene = new BandageGraphicsScene(this);

    g_graphicsViewFeaturesForest->setScene(m_scene);

    g_graphicsViewFeaturesForest->undoRotation();
}

void FeaturesForestWidget::layoutGraph()
{
    //The actual layout is done in a different thread so the UI will stay responsive.
    /*auto *progress = new MyProgressDialog(parent, "Laying out graph...", true, "Cancel layout", "Cancelling layout...",
                                          "Clicking this button will halt the graph layout and display "
                                          "the graph in its current, incomplete state.<br><br>"
                                          "Layout can take a long time for very large graphs.  There are "
                                          "three strategies to reduce the amount of time required:<ul>"
                                          "<li>Change the scope of the graph from 'Entire graph' to either "
                                          "'Around nodes' or 'Around BLAST hits'.  This will reduce the "
                                          "number of nodes that are drawn to the screen.</li>"
                                          "<li>Increase the 'Base pairs per segment' setting.  This will "
                                          "result in shorter contigs which take less time to lay out.</li>"
                                          "<li>Reduce the 'Graph layout iterations' setting.</li></ul>");
    progress->setWindowModality(Qt::WindowModal);
    progress->show();*/

    auto *layoutWorker = new TreeLayoutWorker();

    //connect(progress, SIGNAL(halt()), layoutWorker, SLOT(cancelLayout()));

    auto *watcher = new QFutureWatcher<FeaturesLayout>;

    connect(watcher, &QFutureWatcher<FeaturesLayout>::finished,
            this, [=]() { this->graphLayoutFinished(watcher->future().result()); });
    connect(watcher, SIGNAL(finished()), layoutWorker, SLOT(deleteLater()));
    //connect(watcher, SIGNAL(finished()), progress, SLOT(deleteLater()));
    connect(watcher, SIGNAL(finished()), watcher, SLOT(deleteLater()));

    auto res = QtConcurrent::run(&TreeLayoutWorker::layoutGraph, layoutWorker, std::cref(*g_assemblyFeaturesForest));
    watcher->setFuture(res);
}
