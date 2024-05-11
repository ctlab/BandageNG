// Copyright 2022 Anton Korobeynikov

// This file is part of Bandage

// Bandage is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

// Bandage is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with Bandage.  If not, see <http://www.gnu.org/licenses/>.

#include "io.h"

#include "graph/assemblygraph.h"
#include "graph/debruijnnode.h"
#include "graphlayout.h"

#include "program/settings.h"

#include <QFile>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

namespace layout::io {
    bool save(const QString &filename,
              const GraphLayout &layout) {
        QJsonObject jsonLayout;
        for (const auto &entry: layout) {
            QJsonArray segments;
            for (QPointF point: entry.second)
                segments.append(QJsonArray{point.x(), point.y()});

            jsonLayout[entry.first->getName()] = segments;
        }

        QFile saveFile(filename);
        if (!saveFile.open(QIODevice::WriteOnly))
            return false;

        saveFile.write(QJsonDocument(jsonLayout).toJson());

        return true;
    }

    bool saveGraphList(const QString &filename, QSharedPointer<AssemblyGraphList> graphList, bool isCliMode) {
        QJsonObject jsonLayoutGraphList;
        for (AssemblyGraph* graph : graphList->m_graphMap.values()) {
            GraphLayout layout = isCliMode ? *graph->m_layout : layout::fromGraph(*graph, false);
            QJsonObject jsonLayoutGraph;
            for (const auto &entry: layout) {
                QJsonArray segments;
                for (QPointF point: entry.second)
                    segments.append(QJsonArray{point.x(), point.y()});

                jsonLayoutGraph[entry.first->getName()] = segments;
            }
            jsonLayoutGraphList[graph->getGraphName()] = jsonLayoutGraph;
        }


        QFile saveFile(filename);
        if (!saveFile.open(QIODevice::WriteOnly))
            return false;

        saveFile.write(QJsonDocument(jsonLayoutGraphList).toJson());

        return true;
    }

    bool saveTSV(const QString &filename,
                 const GraphLayout &layout) {
        QFile saveFile(filename);
        if (!saveFile.open(QIODevice::WriteOnly | QIODevice::Text))
            return false;

        QTextStream out(&saveFile);
        for (const auto &entry: layout) {
            const DeBruijnNode *node = entry.first;
            QPointF pos = entry.second.front();
            out << entry.first->getName() << '\t' << pos.x() << '\t' << pos.y() << '\n';
        }

        return true;
    }

    bool saveGraphListTSV(const QString &filename, QSharedPointer<AssemblyGraphList> graphList, bool isCliMode) {
        QFile saveFile(filename);
        if (!saveFile.open(QIODevice::WriteOnly | QIODevice::Text))
            return false;

        QTextStream out(&saveFile);
        for (AssemblyGraph* graph : graphList->m_graphMap.values()) {
            GraphLayout layout = isCliMode ? *graph->m_layout : layout::fromGraph(*graph, false);
            for (const auto &entry: layout) {
                const DeBruijnNode *node = entry.first;
                QPointF pos = entry.second.front();
            }
        }

        return true;
    }

    bool load(const QString &filename,
              GraphLayout &layout) {
        QFile loadFile(filename);
        // FIXME: Switch to Error return object stuff!
        if (!loadFile.open(QIODevice::ReadOnly | QIODevice::Text))
            throw std::runtime_error("cannot open file: " + filename.toStdString());

        QJsonParseError error;
        auto jsonLayoutDoc = QJsonDocument::fromJson(loadFile.readAll(), &error);
        if (error.error != QJsonParseError::NoError)
            throw std::runtime_error(error.errorString().toStdString());

        if (!jsonLayoutDoc.isObject())
            throw std::runtime_error("invalid layout format");

        QJsonObject jsonLayout = jsonLayoutDoc.object();
        const AssemblyGraph &graph = layout.graph();
        for (auto it = jsonLayout.begin(); it != jsonLayout.end(); ++it) {
            QString name = it.key();
            std::vector<DeBruijnNode *> nodes = graph.getNodeFromNameExact(name);
            if (nodes.size() != 1) {
                throw std::runtime_error("graph does not contain node: " + (graph.getNodeNameFromString(name)).toStdString());
            }
            DeBruijnNode * node = nodes[0];
            if (!it.value().isArray())
                throw std::runtime_error("invalid layout format");
            for (const auto &point : it.value().toArray()) {
                QJsonArray pointArray = point.toArray();
                if (pointArray.size() != 2)
                    throw std::runtime_error("invalid layout format: point size is " + std::to_string(pointArray.size()));
                layout.add(node, { pointArray[0].toDouble(), pointArray[1].toDouble() });
            }
        }

        return true;
    }

    bool loadLayoutList(const QString &filename,
                        QSharedPointer<AssemblyGraphList> graphList) {
        QFile loadFile(filename);
        // FIXME: Switch to Error return object stuff!
        if (!loadFile.open(QIODevice::ReadOnly | QIODevice::Text))
            throw std::runtime_error("cannot open file: " + filename.toStdString());

        QJsonParseError error;
        auto jsonLayoutDoc = QJsonDocument::fromJson(loadFile.readAll(), &error);
        if (error.error != QJsonParseError::NoError)
            throw std::runtime_error(error.errorString().toStdString());

        if (!jsonLayoutDoc.isObject())
            throw std::runtime_error("invalid layout format");

        QJsonObject jsonLayout = jsonLayoutDoc.object();

        for (auto graphIt = jsonLayout.begin(); graphIt != jsonLayout.end(); ++graphIt) {
            QString graphName = graphIt.key();
            AssemblyGraph* graph = graphList->getGraphByName(graphName);
            const AssemblyGraph& refGraph = std::cref(*graph);
            if (graph == nullptr) {
                throw std::runtime_error("Graph with name " + graphName.toStdString() + " not found");
            }

            GraphLayout * layout = new GraphLayout(refGraph);
            QJsonObject graphLayout = graphIt.value().toObject();
            for (auto it = graphLayout.begin(); it != graphLayout.end(); ++it) {
                QStringList parts = it.key().split("_");
                parts.pop_front();

                QString nodeName;
                for (auto & part : parts)
                {
                    if (nodeName.length() > 0)
                        nodeName += "_";
                    nodeName += part;
                }

                std::vector<DeBruijnNode *> nodes = graph->getNodeFromNameExact(QString::number(graph->getGraphId()) + "_" + nodeName);
                if (nodes.size() != 1) {
                    throw std::runtime_error("graph does not contain node: " + (graph->getNodeNameFromString(nodeName)).toStdString());
                }
                DeBruijnNode * node = nodes[0];
                if (!it.value().isArray())
                    throw std::runtime_error("invalid layout format");
                for (const auto &point : it.value().toArray()) {
                    QJsonArray pointArray = point.toArray();
                    if (pointArray.size() != 2)
                        throw std::runtime_error("invalid layout format: point size is " + std::to_string(pointArray.size()));
                    layout->add(node, { pointArray[0].toDouble(), pointArray[1].toDouble() });
                }
            }
            layout::apply(*graph, *layout);
            graph->setLayout(layout);
        }
        return true;
    }
}
