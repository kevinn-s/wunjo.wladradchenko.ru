/*
    SPDX-FileCopyrightText: 2026 Streamable Contributors
    SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

/**
 * @file FilmModel.cpp
 * @brief Implements the paginated Qt item model for TMDB movie search results.
 */

#include "FilmModel.h"

#include <QDebug>
#include <QVariant>

#include <limits>
#include <utility>

namespace Streamable {

FilmModel::FilmModel(Tmdb *tmdb, QObject *parent)
    : QAbstractListModel(parent)
    , m_tmdb(tmdb)
{
    if (!m_tmdb)
        return;

    // Queue delivery so searchFilms() returns its identity before any event is handled.
    connect(m_tmdb, &Tmdb::filmsFetched, this, &FilmModel::onFilmsFetched,
            Qt::QueuedConnection);
    connect(m_tmdb, &Tmdb::requestFailed, this, &FilmModel::onRequestFailed,
            Qt::QueuedConnection);
    connect(m_tmdb, &Tmdb::requestStateChanged, this, &FilmModel::onRequestStateChanged,
            Qt::QueuedConnection);
    connect(m_tmdb, &QObject::destroyed, this, [this] {
        m_tmdb = nullptr;
        m_pendingRequestId = Tmdb::InvalidRequestId;
        if (std::exchange(m_loading, false))
            Q_EMIT loadingChanged(false);
    });
}

FilmModel::~FilmModel()
{
    if (m_tmdb && m_pendingRequestId != Tmdb::InvalidRequestId) {
        const Tmdb::RequestId requestId = std::exchange(m_pendingRequestId, Tmdb::InvalidRequestId);
        disconnect(m_tmdb, nullptr, this, nullptr);
        m_tmdb->cancelRequest(requestId);
    }
}

int FilmModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_films.size());
}

QVariant FilmModel::data(const QModelIndex &index, int role) const
{
     if (!index.isValid() || index.row() >= m_films.count())
        return QVariant();

    const Film &film = m_films.at(index.row());

    switch (role) {
        case BackdropPathRole:

    }
    return {};
}

const Film &FilmModel::FilmAt(int row) const
{
    if (row < 0 || row >= m_films.size()) {
        qWarning("FilmModel::FilmAt: invalid row %d", row);
        static const Film empty;
        return empty;
    }
    return m_films.at(row);
}

void FilmModel::setQuery(const QString &query)
{
    const QString committedQuery = query.trimmed();
    if (m_resetting || committedQuery == m_query)
        return;
    resetSearch(committedQuery);
}

void FilmModel::refresh()
{
    if (!m_resetting)
        resetSearch(m_query);
}

void FilmModel::clear()
{
    if (!m_resetting)
        resetSearch({});
}

void FilmModel::resetSearch(const QString &query)
{
    const QString committedQuery = query;
    const bool queryWasChanged = committedQuery != m_query;
    m_resetting = true;
    const Tmdb::RequestId previousRequest = std::exchange(m_pendingRequestId, Tmdb::InvalidRequestId);
    if (m_tmdb && previousRequest != Tmdb::InvalidRequestId)
        m_tmdb->cancelRequest(previousRequest);

    beginResetModel();
    m_films.clear();
    m_query = committedQuery;
    m_currentPage = 0;
    m_totalPages = 0;
    m_totalResults = 0;
    m_lastError = {};
    m_hasError = false;
    const bool wasLoading = std::exchange(m_loading, false);
    endResetModel();
    if (queryWasChanged)
        Q_EMIT queryChanged(m_query);
    if (wasLoading)
        Q_EMIT loadingChanged(false);
    m_resetting = false;
    fetchMore({});
}

QString FilmModel::query() const { return m_query; }
int FilmModel::currentPage() const { return m_currentPage; }
int FilmModel::totalPages() const { return m_totalPages; }
int FilmModel::totalResults() const { return m_totalResults; }
bool FilmModel::isLoading() const { return m_loading; }
Tmdb::Error FilmModel::lastError() const { return m_lastError; }

bool FilmModel::canFetchMore(const QModelIndex &parent) const
{
    return !parent.isValid() && m_tmdb && !m_resetting && !m_loading && !m_hasError
        && !m_query.isEmpty() && (m_currentPage == 0 || m_currentPage < m_totalPages);
}

void FilmModel::fetchMore(const QModelIndex &parent)
{
    if (!canFetchMore(parent))
        return;
    m_loading = true;
    m_pendingRequestId = m_tmdb->searchFilms(m_query, m_currentPage + 1);
    if (m_pendingRequestId == Tmdb::InvalidRequestId) {
        m_loading = false;
        m_hasError = true;
        m_lastError = {Tmdb::ErrorCode::InvalidRequest, 0,
                       tr("The movie search request was not accepted.")};
        Q_EMIT requestFailed(Tmdb::InvalidRequestId, m_lastError);
        return;
    }
    Q_EMIT loadingChanged(true);
}

void FilmModel::onFilmsFetched(Tmdb::RequestId requestId, FilmSearchResult result)
{
    if (requestId == Tmdb::InvalidRequestId || requestId != m_pendingRequestId)
        return;
    if (result.page != m_currentPage + 1 || result.totalPages < 0 || result.totalResults < 0
        || (result.totalPages == 0 && (result.page != 1 || !result.items.isEmpty()))
        || (result.totalPages > 0 && result.page > result.totalPages)
        || result.items.size() > std::numeric_limits<int>::max() - m_films.size()) {
        onRequestFailed(requestId, {Tmdb::ErrorCode::InvalidResponse, 0,
                                   tr("Invalid movie search pagination response.")});
        return;
    }

    // Keep the fetch gate closed until rows and pagination are committed together.
    m_resetting = true;
    const int firstRow = int(m_films.size());
    if (!result.items.isEmpty())
        beginInsertRows({}, firstRow, firstRow + int(result.items.size()) - 1);
    m_films.append(result.items);
    m_currentPage = result.page;
    m_totalPages = result.totalPages;
    m_totalResults = result.totalResults;
    if (!result.items.isEmpty())
        endInsertRows();
    m_pendingRequestId = Tmdb::InvalidRequestId;
    m_loading = false;
    m_resetting = false;
    Q_EMIT loadingChanged(false);
}

void FilmModel::onRequestFailed(Tmdb::RequestId requestId, Tmdb::Error error)
{
    if (requestId == Tmdb::InvalidRequestId || requestId != m_pendingRequestId)
        return;
    m_pendingRequestId = Tmdb::InvalidRequestId;
    m_loading = false;
    m_hasError = true;
    m_lastError = error;
    Q_EMIT loadingChanged(false);
    Q_EMIT requestFailed(requestId, error);
}

void FilmModel::onRequestStateChanged(Tmdb::RequestId requestId, Tmdb::State state)
{
    if (requestId == Tmdb::InvalidRequestId || requestId != m_pendingRequestId)
        return;
    // Ready and Failed precede their payload signals; they must not release the gate.
    if (state != Tmdb::State::Canceled)
        return;
    m_pendingRequestId = Tmdb::InvalidRequestId;
    m_loading = false;
    Q_EMIT loadingChanged(false);
}

} // namespace Streamable
