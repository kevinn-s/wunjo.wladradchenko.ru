/*
    SPDX-FileCopyrightText: 2026 Streamable Contributors
    SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

/**
 * @file DiscoverView.cpp
 * @brief Implements the Streamable movie and TV-series discovery view.
 */

#include "DiscoverView.h"

#include "../Tmdb.h"
#include "delegates/DiscoverDelegate.h"
#include "../models/FilmModel.h"
#include "../models/SeriesModel.h"

#include <QAbstractItemModel>
#include <QCoreApplication>
#include <QFontMetrics>
#include <QLabel>
#include <QListView>
#include <QProgressBar>
#include <QStackedWidget>
#include <QVBoxLayout>

static QFont streamableStatusFont(const QFont &baseFont, int pixelSize, QFont::Weight weight)
{
    QFont font = baseFont;
    font.setFamily(QStringLiteral("Plus Jakarta Sans"));
    font.setPixelSize(pixelSize);
    font.setWeight(weight);
    return font;
}

static QString userFacingError(Streamable::Tmdb::ErrorCode code)
{
    switch (code) {
    case Streamable::Tmdb::ErrorCode::Network:
        return QCoreApplication::translate("Streamable::DiscoverView",
                                           "Connection problem. Check your network and try again.");
    case Streamable::Tmdb::ErrorCode::Authentication:
        return QCoreApplication::translate("Streamable::DiscoverView", "TMDB authentication failed.");
    case Streamable::Tmdb::ErrorCode::Http:
        return QCoreApplication::translate("Streamable::DiscoverView", "TMDB could not complete the search.");
    case Streamable::Tmdb::ErrorCode::InvalidResponse:
    case Streamable::Tmdb::ErrorCode::JsonParsing:
        return QCoreApplication::translate("Streamable::DiscoverView", "TMDB returned an invalid response.");
    case Streamable::Tmdb::ErrorCode::InvalidRequest:
        return QCoreApplication::translate("Streamable::DiscoverView", "Search request could not be started.");
    }
    return QCoreApplication::translate("Streamable::DiscoverView", "Search failed. Try again.");
}

namespace Streamable {

DiscoverView::DiscoverView(Tmdb *tmdb, QWidget *parent)
    : QWidget(parent)
    , m_filmModel(new FilmModel(tmdb, this))
    , m_seriesModel(new SeriesModel(tmdb, this))
    , m_stateStack(new QStackedWidget(this))
    , m_resultView(new QListView(m_stateStack))
    , m_loadingIndicator(new QProgressBar)
    , m_loadingLabel(new QLabel(tr("Searching...")))
    , m_emptyLabel(new QLabel(tr("No results found")))
    , m_errorLabel(new QLabel)
{
    m_resultView->setModel(m_filmModel);
    m_resultView->setItemDelegate(new DiscoverDelegate(m_resultView));
    m_stateStack->addWidget(m_resultView);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_stateStack);

    QWidget *loadingPage = new QWidget(m_stateStack);
    QVBoxLayout *loadingLayout = new QVBoxLayout(loadingPage);
    loadingLayout->setContentsMargins(0, 0, 0, 0);
    loadingLayout->setSpacing(QFontMetrics(font()).height() / 2);
    m_loadingIndicator->setRange(0, 0);
    m_loadingIndicator->setTextVisible(false);
    m_loadingIndicator->setMaximumWidth(240);
    m_loadingIndicator->setAccessibleName(tr("Searching"));
    m_loadingLabel->setAlignment(Qt::AlignCenter);
    m_loadingLabel->setFont(streamableStatusFont(font(), 18, QFont::Medium));
    loadingLayout->addStretch(1);
    loadingLayout->addWidget(m_loadingLabel);
    loadingLayout->addWidget(m_loadingIndicator, 0, Qt::AlignHCenter);
    loadingLayout->addStretch(1);
    m_stateStack->addWidget(loadingPage);

    QWidget *emptyPage = new QWidget(m_stateStack);
    QVBoxLayout *emptyLayout = new QVBoxLayout(emptyPage);
    emptyLayout->setContentsMargins(0, 0, 0, 0);
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_emptyLabel->setFont(streamableStatusFont(font(), 18, QFont::Medium));
    emptyLayout->addWidget(m_emptyLabel);
    m_stateStack->addWidget(emptyPage);

    QWidget *errorPage = new QWidget(m_stateStack);
    QVBoxLayout *errorLayout = new QVBoxLayout(errorPage);
    errorLayout->setContentsMargins(0, 0, 0, 0);
    m_errorLabel->setAlignment(Qt::AlignCenter);
    m_errorLabel->setWordWrap(true);
    m_errorLabel->setFont(streamableStatusFont(font(), 16, QFont::Medium));
    errorLayout->addWidget(m_errorLabel);
    m_stateStack->addWidget(errorPage);

    connect(m_resultView, &QAbstractItemView::activated, this, [this](const QModelIndex &index) {
        if (!index.isValid())
            return;
        if (m_resultView->model() == m_filmModel) {
            if (index.row() >= 0 && index.row() < m_filmModel->rowCount())
                Q_EMIT filmActivated(m_filmModel->FilmAt(index.row()));
            return;
        }
        if (m_resultView->model() == m_seriesModel && index.row() >= 0
            && index.row() < m_seriesModel->rowCount())
            Q_EMIT seriesActivated(m_seriesModel->SeriesAt(index.row()));
    });

    connect(m_filmModel, &FilmModel::loadingChanged, this, [this](bool loading) {
        if (m_resultView->model() == m_filmModel)
            updateModelPresentation(m_filmModel, loading, m_filmModel->rowCount(),
                                    m_filmModel->currentPage());
    });
    connect(m_seriesModel, &SeriesModel::loadingChanged, this, [this](bool loading) {
        if (m_resultView->model() == m_seriesModel)
            updateModelPresentation(m_seriesModel, loading, m_seriesModel->rowCount(),
                                    m_seriesModel->currentPage());
    });
    connect(m_filmModel, &FilmModel::requestFailed, this,
            [this](Tmdb::RequestId, Tmdb::Error error) {
        if (m_resultView->model() == m_filmModel && m_filmModel->rowCount() == 0)
            showError(userFacingError(error.code));
    });
    connect(m_seriesModel, &SeriesModel::requestFailed, this,
            [this](Tmdb::RequestId, Tmdb::Error error) {
        if (m_resultView->model() == m_seriesModel && m_seriesModel->rowCount() == 0)
            showError(userFacingError(error.code));
    });
}

void DiscoverView::searchFilms(const QString &query)
{
    m_resultView->setModel(m_filmModel);
    m_filmModel->setQuery(query);
    m_resultView->scrollToTop();
    updateModelPresentation(m_filmModel, m_filmModel->isLoading(), m_filmModel->rowCount(),
                            m_filmModel->currentPage());
    if (!m_filmModel->isLoading() && m_filmModel->rowCount() == 0
        && m_filmModel->currentPage() == 0 && !m_filmModel->lastError().message.isEmpty())
        showError(userFacingError(m_filmModel->lastError().code));
}

void DiscoverView::searchSeries(const QString &query)
{
    m_resultView->setModel(m_seriesModel);
    m_seriesModel->setQuery(query);
    m_resultView->scrollToTop();
    updateModelPresentation(m_seriesModel, m_seriesModel->isLoading(), m_seriesModel->rowCount(),
                            m_seriesModel->currentPage());
    if (!m_seriesModel->isLoading() && m_seriesModel->rowCount() == 0
        && m_seriesModel->currentPage() == 0 && !m_seriesModel->lastError().message.isEmpty())
        showError(userFacingError(m_seriesModel->lastError().code));
}

void DiscoverView::clear()
{
    m_filmModel->clear();
    m_seriesModel->clear();
    m_resultView->setModel(nullptr);
    m_stateStack->setCurrentWidget(m_resultView);
}

void DiscoverView::updateModelPresentation(const QAbstractItemModel *model, bool loading,
                                           int rowCount, int currentPage)
{
    if (model != m_resultView->model())
        return;
    if (rowCount > 0) {
        m_stateStack->setCurrentWidget(m_resultView);
    } else if (loading) {
        m_stateStack->setCurrentWidget(m_loadingLabel->parentWidget());
    } else if (currentPage > 0) {
        m_stateStack->setCurrentWidget(m_emptyLabel->parentWidget());
    }
}

void DiscoverView::showError(const QString &message)
{
    m_errorLabel->setText(message);
    m_stateStack->setCurrentWidget(m_errorLabel->parentWidget());
}

} // namespace Streamable
