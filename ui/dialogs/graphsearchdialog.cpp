﻿// Copyright 2017 Ryan Wick
// Copyright 2022 Anton Korobeynikov

//This file is part of Bandage-NG

// Bandage is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// Bandage is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with Bandage.  If not, see <http://www.gnu.org/licenses/>.

#include "graphsearchdialog.h"
#include "ui_graphsearchdialog.h"

#include "enteronequerydialog.h"

#include "graphsearch/hit.h"
#include "graphsearch/query.h"
#include "graphsearch/graphsearch.h"

#include "graph/debruijnnode.h"
#include "graph/assemblygraph.h"
#include "graph/annotationsmanager.h"

#include "myprogressdialog.h"
#include "querypathsdialog.h"
#include "hitfiltersdialog.h"

#include "program/globals.h"
#include "program/settings.h"
#include "program/memory.h"

#include <QFileDialog>
#include <QFile>
#include <QString>
#include <QMessageBox>
#include <QCheckBox>
#include <QColorDialog>
#include <QPainter>
#include <QSortFilterProxyModel>
#include <QtConcurrent>

using namespace search;

enum class QueriesHitColumns : unsigned {
    Color = 0,
    Show = 1,
    QueryName = 2,
    Type = 3,
    Length = 4,
    Hits = 5,
    QueryCover = 6,
    Paths = 7,
    TotalHitColumns = Paths + 1
};

enum class HitsColumns : unsigned {
    Color = 0,
    QueryName = 1,
    NodeName = 2,
    PercentIdentity = 3,
    AlignmentLength = 4,
    QueryCover = 5,
    Mismatches = 6,
    GapOpens = 7,
    QueryStart = 8,
    QueryEnd = 9,
    NodeStart = 10,
    NodeEnd = 11,
    Evalue = 12,
    BitScore = 13,
    TotalHitColumns = BitScore + 1
};

GraphSearchDialog::GraphSearchDialog(QWidget *parent, const QString& autoQuery)
 : QDialog(parent), ui(new Ui::GraphSearchDialog) {
    ui->setupUi(this);

    m_graphSearch = search::GraphSearch::get(search::BLAST, QDir::temp(), this);

    setWindowFlags(windowFlags() | Qt::Tool);

    m_queriesListModel = new QueriesListModel(m_graphSearch->queries(),
                                              ui->blastQueriesTable);
    auto *proxyQModel = new QSortFilterProxyModel(ui->blastQueriesTable);
    proxyQModel->setSourceModel(m_queriesListModel);
    ui->blastQueriesTable->setModel(proxyQModel);
    ui->blastQueriesTable->setSortingEnabled(true);

    auto *queryPathsDelegate = new PathButtonDelegate(ui->blastQueriesTable);
    ui->blastQueriesTable->setItemDelegateForColumn(int(QueriesHitColumns::Paths),
                                                    queryPathsDelegate);
    connect(queryPathsDelegate, &PathButtonDelegate::queryPathSelectionChanged,
            [this] { emit queryPathSelectionChanged(); });

    m_hitsListModel = new HitsListModel(m_graphSearch->queries(), ui->blastHitsTable);
    auto *proxyHModel = new QSortFilterProxyModel(ui->blastHitsTable);
    proxyHModel->setSourceModel(m_hitsListModel);
    ui->blastHitsTable->setModel(proxyHModel);
    ui->blastHitsTable->setSortingEnabled(true);

    setFilterText();
    setUiCaptions();

    // Load any previous parameters the user might have entered when previously using this dialog.
    ui->parametersLineEdit->setText(g_settings->blastSearchParameters);

    // If the dialog is given an autoQuery parameter, then it will
    // carry out the entire process on its own.
    if (!autoQuery.isEmpty()) {
        buildDatabase(false);
        clearAllQueries();
        loadQueriesFromFile(autoQuery);
        runGraphSearches(false);
        QMetaObject::invokeMethod(this, "close", Qt::QueuedConnection);
        emit changed();
        return;
    }

    // If a BLAST database already exists, move to step 2.
    if (isDbBuild())
        setUiStep(GRAPH_DB_BUILT_BUT_NO_QUERIES);
    //If there isn't a BLAST database, clear the entire temporary directory
    //and move to step 1.
    else {
        m_graphSearch->emptyTempDirectory();
        setUiStep(GRAPH_DB_NOT_YET_BUILT);
    }

    // If queries already exist, display them and move to step 3.
    if (!m_graphSearch->queries().empty()) {
        updateTables();
        setUiStep(READY_FOR_GRAPH_SEARCH);
    }

    // If results already exist, display them and move to step 4.
    if (!m_hitsListModel->empty()) {
        updateTables();
        setUiStep(GRAPH_SEARCH_COMPLETE);
    }

    connect(ui->buildBlastDatabaseButton, SIGNAL(clicked()), this, SLOT(buildGraphDatabaseInThread()));
    connect(ui->loadQueriesFromFastaButton, SIGNAL(clicked()), this, SLOT(loadQueriesFromFileButtonClicked()));
    connect(ui->enterQueryManuallyButton, SIGNAL(clicked()), this, SLOT(enterQueryManually()));
    connect(ui->clearAllQueriesButton, SIGNAL(clicked()), this, SLOT(clearAllQueries()));
    connect(ui->clearSelectedQueriesButton, SIGNAL(clicked(bool)), this, SLOT(clearSelectedQueries()));
    connect(ui->runBlastSearchButton, SIGNAL(clicked()), this, SLOT(runGraphSearchesInThread()));

    connect(ui->blastQueriesTable->selectionModel(),
        &QItemSelectionModel::selectionChanged,
        [this]() {
            auto *select = ui->blastQueriesTable->selectionModel();
            ui->clearSelectedQueriesButton->setEnabled(select->hasSelection());
        });

    connect(ui->blastQueriesTable,
            &QTableView::clicked,
            m_queriesListModel,
            [this](const QModelIndex &index) {
                if (!index.isValid())
                    return;

                auto column = QueriesHitColumns(index.column());
                if (column != QueriesHitColumns::Color)
                    return;

                if (auto *query = m_queriesListModel->query(index)) {
                    QColor chosenColour = QColorDialog::getColor(query->getColour(),
                                                                 this,
                                                                 "Query color", QColorDialog::ShowAlphaChannel);
                    if (!chosenColour.isValid())
                        return;

                    m_queriesListModel->setColor(index, chosenColour);

                    this->activateWindow(); // FIXME: why do we really need this? :(
                }
            });

    connect(m_queriesListModel, &QueriesListModel::dataChanged,
            [this]() {
                emit changed();
            });

    // This is weird: we need to propagate data changes to proxies
    connect(m_queriesListModel, &QueriesListModel::dataChanged,
            [proxyHModel, proxyQModel](const QModelIndex &topLeft, const QModelIndex &bottomRight) {
                emit proxyQModel->dataChanged(proxyQModel->mapFromSource(topLeft), proxyQModel->mapFromSource(bottomRight));
                emit proxyHModel->dataChanged(QModelIndex(), QModelIndex());
            });

    connect(ui->blastFiltersButton, SIGNAL(clicked(bool)), this, SLOT(openFiltersDialog()));
    connect(ui->searcherComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(searcherChanged()));
}

GraphSearchDialog::~GraphSearchDialog() {
    delete ui;
}

bool GraphSearchDialog::isDbBuild() {
    QFile databaseFile = m_graphSearch->temporaryDir().filePath("all_nodes.fasta");
    return databaseFile.exists();
}

void GraphSearchDialog::afterWindowShow() {
    updateTables();
}

void GraphSearchDialog::clearHits() {
    m_graphSearch->clearHits();
    g_annotationsManager->removeGroupByName(m_graphSearch->annotationGroupName());
    updateTables();
}

void GraphSearchDialog::fillTablesAfterGraphSearch() {
    updateTables();

    if (m_hitsListModel->empty())
        QMessageBox::information(this, "No hits", "No " + m_graphSearch->name() + " hits were found for the given queries and parameters.");
}

void GraphSearchDialog::updateTables() {
    m_queriesListModel->update();
    m_hitsListModel->update(m_graphSearch->queries());
    ui->blastQueriesTable->resizeColumnsToContents();
    ui->blastHitsTable->resizeColumnsToContents();
}

void GraphSearchDialog::buildGraphDatabaseInThread() {
    buildDatabase(true);
}

void GraphSearchDialog::buildDatabase(bool separateThread, bool showProgress) {
    setUiStep(GRAPH_DB_BUILD_IN_PROGRESS);

    auto * progress = new MyProgressDialog(this, "Running " + m_graphSearch->name() + " database...",
                                           separateThread,
                                           "Cancel build",
                                           "Cancelling build...",
                                           "Clicking this button will stop the " + m_graphSearch->name() + " database from being built.");
    progress->setWindowModality(Qt::WindowModal);
    if (showProgress) progress->show();

    connect(m_graphSearch.get(), SIGNAL(finishedDbBuild(QString)), progress, SLOT(deleteLater()));
    connect(m_graphSearch.get(), SIGNAL(finishedDbBuild(QString)), this, SLOT(graphDatabaseBuildFinished(QString)));
    connect(progress, SIGNAL(halt()), m_graphSearch.get(), SLOT(cancelDatabaseBuild()));

    auto builder = [&]() { m_graphSearch->buildDatabase(g_assemblyGraph); };
    if (separateThread) {
        QFuture<void> res = QtConcurrent::run(builder);
    } else
        builder();
}


void GraphSearchDialog::graphDatabaseBuildFinished(const QString& error) {
    disconnect(m_graphSearch.get(), SIGNAL(finishedDbBuild(QString)), this, nullptr);

    if (!error.isEmpty()) {
        QMessageBox::warning(this, "Error", error);
        setUiStep(GRAPH_DB_NOT_YET_BUILT);
    } else
        setUiStep(GRAPH_DB_BUILT_BUT_NO_QUERIES);
}

void GraphSearchDialog::loadQueriesFromFileButtonClicked() {
    QStringList fullFileNames = QFileDialog::getOpenFileNames(this, "Load queries", g_memory->rememberedPath);

    if (fullFileNames.empty()) //User did hit cancel
        return;

    for (const auto &fullFileName : fullFileNames)
        loadQueriesFromFile(fullFileName);
}

void GraphSearchDialog::loadQueriesFromFile(const QString& fullFileName) {
    auto * progress = new MyProgressDialog(this, "Loading queries...", false);
    progress->setWindowModality(Qt::WindowModal);
    progress->show();

    int queriesLoaded = m_graphSearch->loadQueriesFromFile(fullFileName);
    if (queriesLoaded > 0) {
        clearHits();

        g_memory->rememberedPath = QFileInfo(fullFileName).absolutePath();
        setUiStep(READY_FOR_GRAPH_SEARCH);
    }
    updateTables();

    progress->close();
    progress->deleteLater();

    if (queriesLoaded == 0)
        QMessageBox::information(this, "No queries loaded",
                                 "No queries could be loaded from the specified file: " + m_graphSearch->lastError());
}

void GraphSearchDialog::enterQueryManually() {
    EnterOneQueryDialog enterOneQueryDialog(this);
    if (!enterOneQueryDialog.exec())
        return;

    QString queryName = GraphSearch::cleanQueryName(enterOneQueryDialog.getName());
    m_graphSearch->addQuery(new search::Query(queryName,
                                              enterOneQueryDialog.getSequence()));
    updateTables();
    clearHits();

    setUiStep(READY_FOR_GRAPH_SEARCH);
}

void GraphSearchDialog::clearAllQueries() {
    ui->clearAllQueriesButton->setEnabled(false);

    m_queriesListModel->m_queries.get().clearAllQueries();

    clearHits();
    updateTables();

    setUiStep(GRAPH_DB_BUILT_BUT_NO_QUERIES);
    emit changed();
}

void GraphSearchDialog::clearSelectedQueries() {
    // Use the table selection to figure out which queries are to be removed.
    QItemSelectionModel *select = ui->blastQueriesTable->selectionModel();
    QModelIndexList selection = select->selectedIndexes();

    if (selection.size() == m_queriesListModel->m_queries.get().getQueryCount()) {
        clearAllQueries();
        return;
    }

    std::vector<Query *> queriesToRemove;
    for (const auto &index : selection)
        queriesToRemove.push_back(m_queriesListModel->query(index));
    m_queriesListModel->m_queries.get().clearSomeQueries(queriesToRemove);

    updateTables();

    emit changed();
}

void GraphSearchDialog::runGraphSearchesInThread() {
    runGraphSearches(true);
}

void GraphSearchDialog::runGraphSearches(bool separateThread, QString extraParameters, bool showProgress) {
    setUiStep(GRAPH_SEARCH_IN_PROGRESS);

    clearHits();

    auto * progress = new MyProgressDialog(this, "Running " + m_graphSearch->name() + " search...",
                                           separateThread,
                                           "Cancel search",
                                           "Cancelling search...",
                                           "Clicking this button will stop the " + m_graphSearch->name() + " search.");
    progress->setWindowModality(Qt::WindowModal);
    if (showProgress) progress->show();

    connect(m_graphSearch.get(), SIGNAL(finishedSearch(QString)), progress, SLOT(deleteLater()));
    connect(m_graphSearch.get(), SIGNAL(finishedSearch(QString)), this, SLOT(graphSearchFinished(QString)));
    connect(progress, SIGNAL(halt()), m_graphSearch.get(), SLOT(cancelSearch()));

    if (extraParameters.isEmpty()) {
        extraParameters = ui->parametersLineEdit->text().simplified();
    }

    auto searcher = [&]() { m_graphSearch->doSearch(extraParameters); };
    if (separateThread) {
        QFuture<void> res = QtConcurrent::run(searcher);
    } else
        searcher();
}

void GraphSearchDialog::graphSearchFinished(const QString& error) {
    disconnect(m_graphSearch.get(), SIGNAL(finishedSearch(QString)), this, nullptr);

    if (!error.isEmpty()) {
        QMessageBox::warning(this, "Error", error);
        setUiStep(READY_FOR_GRAPH_SEARCH);
    } else {
        fillTablesAfterGraphSearch();
        g_settings->blastSearchParameters = ui->parametersLineEdit->text().simplified();
        setUiStep(GRAPH_SEARCH_COMPLETE);
    }

    emit changed();
}

void GraphSearchDialog::setUiStep(SearchUiState uiState) {
    QPixmap tick(":/icons/tick-128.png");
    tick.setDevicePixelRatio(devicePixelRatio()); //This is a workaround for a Qt bug.  Can possibly remove in the future.  https://bugreports.qt.io/browse/QTBUG-46846

    switch (uiState) {
    case GRAPH_DB_NOT_YET_BUILT:
        ui->step1Label->setEnabled(true);
        ui->buildBlastDatabaseButton->setEnabled(true);
        ui->step2Label->setEnabled(false);
        ui->loadQueriesFromFastaButton->setEnabled(false);
        ui->enterQueryManuallyButton->setEnabled(false);
        ui->blastQueriesTable->setEnabled(false);
        ui->blastQueriesTableInfoText->setEnabled(false);
        ui->step3Label->setEnabled(false);
        ui->parametersLabel->setEnabled(false);
        ui->parametersLineEdit->setEnabled(false);
        ui->runBlastSearchButton->setEnabled(false);
        ui->clearAllQueriesButton->setEnabled(false);
        ui->clearSelectedQueriesButton->setEnabled(false);
        ui->hitsLabel->setEnabled(false);
        ui->step1TickLabel->setPixmap(QPixmap());
        ui->step2TickLabel->setPixmap(QPixmap());
        ui->step3TickLabel->setPixmap(QPixmap());
        ui->buildBlastDatabaseInfoText->setEnabled(true);
        ui->loadQueriesFromFastaInfoText->setEnabled(false);
        ui->enterQueryManuallyInfoText->setEnabled(false);
        ui->clearAllQueriesInfoText->setEnabled(false);
        ui->clearSelectedQueriesInfoText->setEnabled(false);
        ui->blastHitsTable->setEnabled(false);
        ui->blastSearchWidget->setEnabled(false);
        ui->blastHitsTableInfoText->setEnabled(false);
        break;

    case GRAPH_DB_BUILD_IN_PROGRESS:
        ui->step1Label->setEnabled(true);
        ui->buildBlastDatabaseButton->setEnabled(false);
        ui->step2Label->setEnabled(false);
        ui->loadQueriesFromFastaButton->setEnabled(false);
        ui->enterQueryManuallyButton->setEnabled(false);
        ui->blastQueriesTable->setEnabled(false);
        ui->blastQueriesTableInfoText->setEnabled(false);
        ui->step3Label->setEnabled(false);
        ui->parametersLabel->setEnabled(false);
        ui->parametersLineEdit->setEnabled(false);
        ui->runBlastSearchButton->setEnabled(false);
        ui->clearAllQueriesButton->setEnabled(false);
        ui->clearSelectedQueriesButton->setEnabled(false);
        ui->hitsLabel->setEnabled(false);
        ui->step1TickLabel->setPixmap(QPixmap());
        ui->step2TickLabel->setPixmap(QPixmap());
        ui->step3TickLabel->setPixmap(QPixmap());
        ui->buildBlastDatabaseInfoText->setEnabled(false);
        ui->loadQueriesFromFastaInfoText->setEnabled(false);
        ui->enterQueryManuallyInfoText->setEnabled(false);
        ui->clearAllQueriesInfoText->setEnabled(false);
        ui->clearSelectedQueriesInfoText->setEnabled(false);
        ui->blastHitsTable->setEnabled(false);
        ui->blastSearchWidget->setEnabled(false);
        ui->blastHitsTableInfoText->setEnabled(false);
        break;

    case GRAPH_DB_BUILT_BUT_NO_QUERIES:
        ui->step1Label->setEnabled(true);
        ui->buildBlastDatabaseButton->setEnabled(false);
        ui->step2Label->setEnabled(true);
        ui->loadQueriesFromFastaButton->setEnabled(true);
        ui->enterQueryManuallyButton->setEnabled(m_graphSearch->allowManualQueries());
        ui->blastQueriesTable->setEnabled(true);
        ui->blastQueriesTableInfoText->setEnabled(true);
        ui->step3Label->setEnabled(false);
        ui->parametersLabel->setEnabled(false);
        ui->parametersLineEdit->setEnabled(false);
        ui->runBlastSearchButton->setEnabled(false);
        ui->clearAllQueriesButton->setEnabled(false);
        ui->clearAllQueriesButton->setEnabled(false);
        ui->hitsLabel->setEnabled(false);
        ui->step1TickLabel->setPixmap(tick);
        ui->step2TickLabel->setPixmap(QPixmap());
        ui->step3TickLabel->setPixmap(QPixmap());
        ui->buildBlastDatabaseInfoText->setEnabled(true);
        ui->loadQueriesFromFastaInfoText->setEnabled(true);
        ui->enterQueryManuallyInfoText->setEnabled(true);
        ui->clearSelectedQueriesInfoText->setEnabled(false);
        ui->clearSelectedQueriesInfoText->setEnabled(false);
        ui->blastHitsTable->setEnabled(false);
        ui->blastSearchWidget->setEnabled(false);
        ui->blastHitsTableInfoText->setEnabled(false);
        break;

    case READY_FOR_GRAPH_SEARCH:
        ui->step1Label->setEnabled(true);
        ui->buildBlastDatabaseButton->setEnabled(false);
        ui->step2Label->setEnabled(true);
        ui->loadQueriesFromFastaButton->setEnabled(true);
        ui->enterQueryManuallyButton->setEnabled(m_graphSearch->allowManualQueries());
        ui->blastQueriesTable->setEnabled(true);
        ui->blastQueriesTableInfoText->setEnabled(true);
        ui->step3Label->setEnabled(true);
        ui->parametersLabel->setEnabled(true);
        ui->parametersLineEdit->setEnabled(true);
        ui->runBlastSearchButton->setEnabled(true);
        ui->clearAllQueriesButton->setEnabled(true);
        ui->hitsLabel->setEnabled(false);
        ui->step1TickLabel->setPixmap(tick);
        ui->step2TickLabel->setPixmap(tick);
        ui->step3TickLabel->setPixmap(QPixmap());
        ui->buildBlastDatabaseInfoText->setEnabled(true);
        ui->loadQueriesFromFastaInfoText->setEnabled(true);
        ui->enterQueryManuallyInfoText->setEnabled(true);
        ui->clearAllQueriesInfoText->setEnabled(true);
        ui->clearSelectedQueriesInfoText->setEnabled(true);
        ui->blastHitsTable->setEnabled(false);
        ui->blastSearchWidget->setEnabled(true);
        ui->blastHitsTableInfoText->setEnabled(false);
        break;

    case GRAPH_SEARCH_IN_PROGRESS:
        ui->step1Label->setEnabled(true);
        ui->buildBlastDatabaseButton->setEnabled(false);
        ui->step2Label->setEnabled(true);
        ui->loadQueriesFromFastaButton->setEnabled(true);
        ui->enterQueryManuallyButton->setEnabled(m_graphSearch->allowManualQueries());
        ui->blastQueriesTable->setEnabled(true);
        ui->blastQueriesTableInfoText->setEnabled(true);
        ui->step3Label->setEnabled(true);
        ui->parametersLabel->setEnabled(true);
        ui->parametersLineEdit->setEnabled(true);
        ui->runBlastSearchButton->setEnabled(false);
        ui->clearAllQueriesButton->setEnabled(true);
        ui->hitsLabel->setEnabled(false);
        ui->step1TickLabel->setPixmap(tick);
        ui->step2TickLabel->setPixmap(tick);
        ui->step3TickLabel->setPixmap(QPixmap());
        ui->buildBlastDatabaseInfoText->setEnabled(true);
        ui->loadQueriesFromFastaInfoText->setEnabled(true);
        ui->enterQueryManuallyInfoText->setEnabled(true);
        ui->clearAllQueriesInfoText->setEnabled(true);
        ui->clearSelectedQueriesInfoText->setEnabled(true);
        ui->blastHitsTable->setEnabled(false);
        ui->blastSearchWidget->setEnabled(true);
        ui->blastHitsTableInfoText->setEnabled(false);
        break;

    case GRAPH_SEARCH_COMPLETE:
        ui->step1Label->setEnabled(true);
        ui->buildBlastDatabaseButton->setEnabled(false);
        ui->step2Label->setEnabled(true);
        ui->loadQueriesFromFastaButton->setEnabled(true);
        ui->enterQueryManuallyButton->setEnabled(m_graphSearch->allowManualQueries());
        ui->blastQueriesTable->setEnabled(true);
        ui->blastQueriesTableInfoText->setEnabled(true);
        ui->step3Label->setEnabled(true);
        ui->parametersLabel->setEnabled(true);
        ui->parametersLineEdit->setEnabled(true);
        ui->runBlastSearchButton->setEnabled(true);
        ui->clearAllQueriesButton->setEnabled(true);
        ui->hitsLabel->setEnabled(true);
        ui->step1TickLabel->setPixmap(tick);
        ui->step2TickLabel->setPixmap(tick);
        ui->step3TickLabel->setPixmap(tick);
        ui->buildBlastDatabaseInfoText->setEnabled(true);
        ui->loadQueriesFromFastaInfoText->setEnabled(true);
        ui->enterQueryManuallyInfoText->setEnabled(true);
        ui->clearAllQueriesInfoText->setEnabled(true);
        ui->clearSelectedQueriesInfoText->setEnabled(true);
        ui->blastHitsTable->setEnabled(true);
        ui->blastSearchWidget->setEnabled(true);
        ui->blastHitsTableInfoText->setEnabled(true);
        break;
    }
}

void GraphSearchDialog::openFiltersDialog() {
    HitFiltersDialog filtersDialog(this);
    filtersDialog.setWidgetsFromSettings();

    if (!filtersDialog.exec())
        return; //The user did not click OK

    filtersDialog.setSettingsFromWidgets();
    setFilterText();
}

void GraphSearchDialog::setFilterText() {
    ui->blastHitFiltersLabel->setText("Current filters: " + HitFiltersDialog::getFilterText());
}

void GraphSearchDialog::setUiCaptions() {
    ui->step1Label->setText(QString("<b>Step 1:</b> build %1 database").arg(m_graphSearch->name()));
    ui->step2Label->setText(QString("<b>Step 2:</b> enter %1 queries").arg(m_graphSearch->name()));
    ui->step3Label->setText(QString("<b>Step 3:</b> run %1 search").arg(m_graphSearch->name()));
    ui->loadQueriesFromFastaButton->setText(QString("Load from %1 file").arg(m_graphSearch->queryFormat()));
    ui->buildBlastDatabaseButton->setText(QString("Build %1 database").arg(m_graphSearch->name()));
    ui->blastFiltersButton->setText(QString("Set %1 hit filters").arg(m_graphSearch->name()));
    ui->runBlastSearchButton->setText(QString("Run %1 search").arg(m_graphSearch->name()));
}

void GraphSearchDialog::searcherChanged() {
    m_queriesListModel->startUpdate();
    g_annotationsManager->removeGroupByName(m_graphSearch->annotationGroupName());

    m_graphSearch = search::GraphSearch::get(search::GraphSearchKind(ui->searcherComboBox->currentIndex()),
                                             QDir::temp(), this);
    m_queriesListModel->setQueries(m_graphSearch->queries());

    m_queriesListModel->endUpdate();

    setUiStep(GRAPH_DB_NOT_YET_BUILT);
    setUiCaptions();

    updateTables();
    emit changed();
}

QueriesListModel::QueriesListModel(Queries &queries, QObject *parent)
  : m_queries(queries), QAbstractTableModel(parent) {}

int QueriesListModel::rowCount(const QModelIndex &) const {
    return m_queries.get().getQueryCount();
}

int QueriesListModel::columnCount(const QModelIndex &) const {
    return int(QueriesHitColumns::TotalHitColumns);
}

QVariant QueriesListModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid())
        return {};

    auto *query = this->query(index);
    if (!query)
        return {};

    auto column = QueriesHitColumns(index.column());
    if (role == Qt::BackgroundRole) {
        if (query->isHidden()) // Hide disabled queries
            return QColor(150, 150, 150);
        else if (column == QueriesHitColumns::Color)
            return query->getColour();
    }

    if (role == Qt::CheckStateRole) {
        if (column == QueriesHitColumns::Show)
            return query->isShown() ? Qt::Checked : Qt::Unchecked;
    }

    if (role == Qt::TextAlignmentRole) {
        if (column == QueriesHitColumns::Show)
            return Qt::AlignCenter;
    }

    if (role == Qt::EditRole && column == QueriesHitColumns::QueryName)
        return query->getName();

    if (role != Qt::DisplayRole)
        return {};

    switch (column) {
        default:
            return {};
        case QueriesHitColumns::QueryName:
            return query->getName();
        case QueriesHitColumns::Type:
            return query->getTypeString();
        case QueriesHitColumns::Length:
            return unsigned(query->getLength());
        case QueriesHitColumns::Hits:
            if (query->wasSearchedFor())
                return unsigned(query->hitCount());
            return "-";
        case QueriesHitColumns::QueryCover:
            if (query->wasSearchedFor())
                return formatDoubleForDisplay(100.0 * query->fractionCoveredByHits(), 2) + "%";
            return "-";
        case QueriesHitColumns::Paths:
            if (query->wasSearchedFor())
                return unsigned(query->getPathCount());
            return "-";
    }
};

QVariant QueriesListModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role == Qt::TextAlignmentRole && orientation == Qt::Horizontal)
        return Qt::AlignCenter;

    if (role != Qt::DisplayRole)
        return {};

    if (orientation == Qt::Vertical)
        return QString::number(section + 1);

    switch (QueriesHitColumns(section)) {
        default:
            return {};
        case QueriesHitColumns::Show:
            return "Show";
        case QueriesHitColumns::QueryName:
            return "Query name";
        case QueriesHitColumns::Type:
            return "Type";
        case QueriesHitColumns::Length:
            return "Length";
        case QueriesHitColumns::Hits:
            return "Hits";
        case QueriesHitColumns::QueryCover:
            return "Query cover";
        case QueriesHitColumns::Paths:
            return "Paths";
    }
}

Qt::ItemFlags QueriesListModel::flags(const QModelIndex &index) const {
    if (!index.isValid())
        return Qt::ItemIsEnabled;

    auto column = QueriesHitColumns(index.column());
    if (column == QueriesHitColumns::Show)
        return QAbstractTableModel::flags(index) | Qt::ItemIsUserCheckable;
    else if (column == QueriesHitColumns::QueryName)
        return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
    else if (column == QueriesHitColumns::Color || column == QueriesHitColumns::Paths)
        return Qt::ItemIsEnabled;

    return QAbstractTableModel::flags(index);
}

bool QueriesListModel::setData(const QModelIndex &index, const QVariant &value, int role) {
    if (!index.isValid())
        return false;

    auto *query = this->query(index);
    if (!query)
        return false;

    auto column = QueriesHitColumns(index.column());
    if (role == Qt::CheckStateRole && column == QueriesHitColumns::Show) { // Implement query shown checkbox
        query->setShown(value.toBool());
        emit dataChanged(index, QModelIndex()); // Here we need to refresh the whole row
        return true;
    } else if (role == Qt::EditRole && column == QueriesHitColumns::QueryName) { // Change query name
        QString newName = value.toString();
        if (newName != query->getName()) {
            m_queries.get().renameQuery(query, newName);
            emit dataChanged(index, index);
            return true;
        }
    }

    return false;
}

Query *QueriesListModel::query(const QModelIndex &index) const {
    if (!index.isValid() || index.row() >= m_queries.get().getQueryCount())
        return nullptr;

    return m_queries.get().query(index.row());
}

void QueriesListModel::setColor(const QModelIndex &index, QColor color) {
    if (!index.isValid())
        return;

    auto column = QueriesHitColumns(index.column());
    if (column != QueriesHitColumns::Color)
        return;

    if (auto *query = this->query(index)) {
        query->setColour(color);
        emit dataChanged(index, index);
    };
}

void PathButtonDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const {
    const auto *model = qobject_cast<const QSortFilterProxyModel*>(index.model());
    auto *query = qobject_cast<const QueriesListModel*>(model->sourceModel())->query(model->mapToSource(index));
    if (query && query->wasSearchedFor()) {
        QStyleOptionButton btn;
        btn.features = QStyleOptionButton::None;
        btn.rect = option.rect;
        btn.state = option.state | QStyle::State_Enabled | QStyle::State_Raised;
        btn.text = QString::number(query->getPathCount());

        QStyle *style = option.widget ? option.widget->style() : QApplication::style();
        style->drawControl(QStyle::CE_PushButton, &btn, painter);
        return;
    }

    QStyledItemDelegate::paint(painter, option, index);
}

bool PathButtonDelegate::editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option,
                                     const QModelIndex &index) {
    if (event->type() == QEvent::MouseButtonRelease) {
        const auto *proxyModel = qobject_cast<const QSortFilterProxyModel*>(model);
        auto *query = qobject_cast<const QueriesListModel*>(proxyModel->sourceModel())->query(proxyModel->mapToSource(index));
        if (query && query->wasSearchedFor()) {
            auto *queryPathsDialog = new QueryPathsDialog(query, nullptr);

            connect(queryPathsDialog,
                    &QueryPathsDialog::selectionChanged,
                    [this]() {
                        emit queryPathSelectionChanged();
                    });

            connect(queryPathsDialog, &QueryPathsDialog::finished,
                    queryPathsDialog, &QueryPathsDialog::deleteLater);

            queryPathsDialog->show();
        }
    }

    return QStyledItemDelegate::editorEvent(event, model, option, index);
}

HitsListModel::HitsListModel(Queries &queries, QObject *parent)
 : QAbstractTableModel(parent) {
    update(queries);
}

int HitsListModel::columnCount(const QModelIndex &) const {
    return int(HitsColumns::TotalHitColumns);
}

int HitsListModel::rowCount(const QModelIndex &) const {
    return m_hits.size();
}

QVariant HitsListModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() >= m_hits.size())
        return {};

    auto column = HitsColumns(index.column());
    const auto &hit = m_hits[index.row()];
    const auto &hitQuery = *hit->m_query;

    if (role == Qt::BackgroundRole) {
        if (hitQuery.isHidden()) // Hide disabled queries
            return QColor(150, 150, 150);
        else if (column == HitsColumns::Color)
            return hitQuery.getColour();
    }

    if (role != Qt::DisplayRole)
        return {};

    switch (column) {
        default:
            return {};
        case HitsColumns::QueryName:
            return hitQuery.getName();
        case HitsColumns::NodeName:
            return hit->m_node->getName();
        case HitsColumns::PercentIdentity:
            if (hit->m_percentIdentity > 0)
                return formatDoubleForDisplay(hit->m_percentIdentity, 2) + "%";
            return "N/A";
        case HitsColumns::AlignmentLength:
            return hit->m_alignmentLength;
        case HitsColumns::QueryCover:
            return formatDoubleForDisplay(100.0 * hit->getQueryCoverageFraction(), 2) + "%";
        case HitsColumns::Mismatches:
            if (hit->m_numberMismatches < 0)
                return "N/A";
            return hit->m_numberMismatches;
        case HitsColumns::GapOpens:
            if (hit->m_numberGapOpens < 0)
                return "N/A";
            return hit->m_numberGapOpens;
        case HitsColumns::QueryStart:
            return hit->m_queryStart;
        case HitsColumns::QueryEnd:
            return hit->m_queryEnd;
        case HitsColumns::NodeStart:
            return hit->m_nodeStart;
        case HitsColumns::NodeEnd:
            return hit->m_nodeEnd;
        case HitsColumns::Evalue:
            return hit->m_eValue.asString(false);
        case HitsColumns::BitScore:
            return hit->m_bitScore;
    }

    return {};
}

QVariant HitsListModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role == Qt::TextAlignmentRole && orientation == Qt::Horizontal)
        return Qt::AlignCenter;

    if (role != Qt::DisplayRole)
        return {};

    if (orientation == Qt::Vertical)
        return QString::number(section + 1);

    switch (HitsColumns(section)) {
        default:
            return {};
        case HitsColumns::QueryName:
            return "Query\nname";
        case HitsColumns::NodeName:
            return "Node\nname";
        case HitsColumns::PercentIdentity:
            return "Percent\nidentity";
        case HitsColumns::AlignmentLength:
            return "Alignment\nlength";
        case HitsColumns::QueryCover:
            return "Query\ncover";
        case HitsColumns::Mismatches:
            return "Mis-\nmatches";
        case HitsColumns::GapOpens:
            return "Gap\nopens";
        case HitsColumns::QueryStart:
            return "Query\nstart";
        case HitsColumns::QueryEnd:
            return "Query\nend";
        case HitsColumns::NodeStart:
            return "Node\nstart";
        case HitsColumns::NodeEnd:
            return "Node\nend";
        case HitsColumns::Evalue:
            return "E-\nvalue";
        case HitsColumns::BitScore:
            return "Bit\nscore";
    }
}

void HitsListModel::update(Queries &queries) {
    startUpdate();
    clear();

    m_hits = queries.allHits();

    endUpdate();
}
