// Copyright 2022 Anton Korobeynikov
// Copyright 2022 Andrei Zakharov

// This file is part of Bandage-NG

// Bandage-NG is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// Bandage-NG is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with Bandage.  If not, see <http://www.gnu.org/licenses/>.

#include "annotationsmanager.h"

#include "graphsearch/query.h"
#include "program/settings.h"

AnnotationGroup &AnnotationsManager::createAnnotationGroup(QString name) {
    if (name == g_settings->blastAnnotationGroupName) {
        // To be removed when we can set up annotations from CLI properly.
        return createAnnotationGroup(name, g_settings->defaultBlastAnnotationSetting);
    }
    return createAnnotationGroup(name, AnnotationSetting());
}

AnnotationGroup &AnnotationsManager::createAnnotationGroup(QString name, const AnnotationSetting &setting) {
    g_settings->annotationsSettings[nextFreeId] = setting;
    m_annotationGroups.emplace_back(
            std::make_unique<AnnotationGroup>(AnnotationGroup{nextFreeId, std::move(name), {}}));
    nextFreeId++;
    emit annotationGroupsUpdated();
    return *m_annotationGroups.back();
}

const AnnotationsManager::AnnotationGroupVector &AnnotationsManager::getGroups() const {
    return m_annotationGroups;
}


void AnnotationsManager::removeGroupByName(const QString &name) {
    auto newEnd = std::remove_if(
            m_annotationGroups.begin(),
            m_annotationGroups.end(),
            [&name](const std::unique_ptr<AnnotationGroup> &group) {
                return group->name == name;
            });
    m_annotationGroups.erase(newEnd, m_annotationGroups.end());
}


const AnnotationGroup *AnnotationsManager::findGroupByName(const QString &name) const {
    auto res = std::find_if(
            m_annotationGroups.begin(),
            m_annotationGroups.end(),
            [&name](const std::unique_ptr<AnnotationGroup> &group) {
                return group->name == name;
            });

    if (res == m_annotationGroups.end())
        return nullptr;

    return res->get();
}

const AnnotationGroup *AnnotationsManager::findGroupById(AnnotationGroupId id) const {
    auto res = std::find_if(
            m_annotationGroups.begin(),
            m_annotationGroups.end(),
            [id](const std::unique_ptr<AnnotationGroup> &group) {
                return group->id == id;
            });

    if (res == m_annotationGroups.end())
        return nullptr;

    return res->get();
}

void AnnotationsManager::updateGroupFromHits(const QString &name, const std::vector<search::Query *> &queries) {
    // Preserve annotation settings, if they existed
    AnnotationSetting groupSettings;
    if (const auto *group = findGroupByName(name))
        groupSettings = g_settings->annotationsSettings[group->id];

    removeGroupByName(name);

    if (queries.empty())
        return;

    auto &group = createAnnotationGroup(name);
    for (auto *query: queries) {
        for (const auto &hit: query->getHits()) {
            auto &annotation = group.annotationMap[hit->m_node].emplace_back(
                    std::make_unique<Annotation>(
                            hit->m_nodeStart,
                            hit->m_nodeEnd,
                            query->getName().toStdString()));
            annotation->addView(std::make_unique<SolidView>(1.0, query->getColour()));
            annotation->addView(std::make_unique<RainbowBlastHitView>(hit->queryStartFraction(),
                                                                      hit->queryEndFraction()));
        }
    }

    g_settings->annotationsSettings[group.id] = groupSettings;

    emit annotationGroupsUpdated();
}

void AnnotationsManager::updateGroupFromHits(const QString &name, const std::vector<search::Query *> &queries, QString typeName) {
    // Preserve annotation settings, if they existed
    AnnotationSetting groupSettings;
    if (const auto *group = findGroupByName(name))
        groupSettings = g_settings->annotationsSettings[group->id];

    removeGroupByName(name);

    if (queries.empty())
        return;

    auto &group = createAnnotationGroup(name);
    for (auto *query: queries) {
        for (const auto &hit: query->getHits()) {
            auto &annotation = group.annotationMap[hit->m_node].emplace_back(
                    std::make_unique<Annotation>(
                            hit->m_nodeStart,
                            hit->m_nodeEnd,
                            query->getName().toStdString()));
            annotation->addView(std::make_unique<SolidView>(1.0, query->getColour()));
            annotation->addView(std::make_unique<FeatureClassView>(1.0, query->getFeatureClassColour()));
            annotation->addView(std::make_unique<RainbowBlastHitView>(hit->queryStartFraction(),
                                                                      hit->queryEndFraction()));
        }
    }

    g_settings->annotationsSettings[group.id] = groupSettings;

    const auto &annotationGroup = *g_annotationsManager->findGroupById(group.id);
    if (!annotationGroup.annotationMap.empty() && !annotationGroup.annotationMap.begin()->second.empty()) {
        // All annotations in group have the same types of views, so we can get view's names from any annotation
        ViewId i = 0;
        for (const auto &view: annotationGroup.annotationMap.begin()->second.front()->getViews()) {

            if (view->getTypeName() == typeName) {
                auto &viewsToShow = g_settings->annotationsSettings[group.id].viewsToShow;
                viewsToShow.clear();
                viewsToShow.insert(i);
            }
            i++;
        }
    }
    emit annotationGroupsUpdated();
}
