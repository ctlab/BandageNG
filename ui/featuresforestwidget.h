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


#ifndef FEATURESFORESTWIDGET_H
#define FEATURESFORESTWIDGET_H

#include <QMainWindow>
#include <QWindow>
#include <QDialog>
#include <QGraphicsScene>
#include <QMap>
#include <QString>
#include <vector>
#include <QLineEdit>
#include <QRectF>
#include "program/globals.h"
#include <QThread>
#include "layout/featureslayout.h"
#include "ui/mainwindow.h"

class GraphicsViewZoom;
class BandageGraphicsScene;
class FeatureTreeNode;
class DeBruijnNode;
class MainWindow;

class FeaturesForestWidget : public QObject
{
    Q_OBJECT

public:
    explicit FeaturesForestWidget(MainWindow *parent);
    ~FeaturesForestWidget();

    void cleanUp();
    void drawGraph(QWidget * parent);

    BandageGraphicsScene * m_scene;

    GraphicsViewZoom * m_graphicsViewZoom;
    QThread * m_layoutThread;
    QString m_imageFilter;
    QString m_fileToLoadOnStartup;
    bool m_drawGraphAfterLoad;
    bool m_alreadyShown;
    double m_previousFeaturesZoomSpinBoxValue;

    void resetScene();
    void layoutGraph();
    void getSelectedNodeInfo(int & selectedNodeCount, QString & selectedFeatureNodeText);
private:
    MainWindow * mainWindow;
private slots:

    void graphLayoutFinished(const FeaturesLayout &layout);
    void graphLayoutCancelled();

signals:
      void windowLoaded();
};

#endif // FEATURESFORESTWIDGET_H
