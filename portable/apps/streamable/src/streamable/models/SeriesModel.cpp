/*
    SPDX-FileCopyrightText: 2026 Streamable Contributors
    SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

/**
 * @file SeriesModel.cpp
 * @brief Implements the infinite-scrolling Qt item model for TMDB TV-series
 *        search results.
 */

#include "SeriesModel.h"

#include <QDebug>
#include <QVariant>

#include <limits>
#include <utility>

namespace Streamable {

SeriesModel::SeriesModel(Tmdb *tmdb, QObject *parent)
    : QAbstractListModel(parent)
    , m_tmdb(tmdb)
{
    if (!m_tmdb)
        return;

    connect(m_tmdb, &Tmdb::seriesFetched, this, &SeriesModel::onSeriesFetched,
            Qt::QueuedConnection);
    connect(m_tmdb, &Tmdb::requestFailed, this, &SeriesModel::onRequestFailed,
            Qt::QueuedConnection);
    connect(m_tmdb, &Tmdb::requestStateChanged, this, &SeriesModel::onRequestStateChanged,
            Qt::QueuedConnection);
    connect(m_tmdb, &QObject::destroyed, this, [this] {
        m_tmdb = nullptr;
        m_pendingRequestId = Tmdb::InvalidRequestId;
        if (std::exchange(m_loading, false))
            Q_EMIT loadingChanged(false);
    });
}

SeriesModel::~SeriesModel()
{
    if (m_tmdb && m_pendingRequestId != Tmdb::InvalidRequestId) {
        const Tmdb::RequestId requestId = std::exchange(m_pendingRequestId, Tmdb::InvalidRequestId);
        disconnect(m_tmdb, nullptr, this, nullptr);
        m_tmdb->cancelRequest(requestId);
    }
}

int SeriesModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_series.size());
}

QVariant SeriesModel::data(const QModelIndex &, int) const
{
    return {};
}

const Series &SeriesModel::SeriesAt(int row) const
{
    if (row < 0 || row >= m_series.size()) {
        qWarning("SeriesModel::SeriesAt: invalid row %d", row);
        static const Series empty;
        return empty;
    }
    return m_series.at(row);
}

void SeriesModel::setQuery(const QString &query)
{
    const QString committedQuery = query.trimmed();
    if (m_resetting || committedQuery == m_query)
        return;
    resetSearch(committedQuery);
}

void SeriesModel::refresh()
{
    if (!m_resetting)
        resetSearch(m_query);
}

void SeriesModel::clear()
{
    if (!m_resetting)
        resetSearch({});
}

void SeriesModel::resetSearch(const QString &query)
{
    const QString committedQuery = query;
    const bool queryWasChanged = committedQuery != m_query;
    m_resetting = true;
    const Tmdb::RequestId previousRequest = std::exchange(m_pendingRequestId, Tmdb::InvalidRequestId);
    if (m_tmdb && previousRequest != Tmdb::InvalidRequestId)
        m_tmdb->cancelRequest(previousRequest);

    beginResetModel();
    m_series.clear();
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

QString SeriesModel::query() const { return m_query; }
int SeriesModel::currentPage() const { return m_currentPage; }
int SeriesModel::totalPages() const { return m_totalPages; }
int SeriesModel::totalResults() const { return m_totalResults; }
bool SeriesModel::isLoading() const { return m_loading; }
Tmdb::Error SeriesModel::lastError() const { return m_lastError; }

bool SeriesModel::canFetchMore(const QModelIndex &parent) const
{
    return !parent.isValid() && m_tmdb && !m_resetting && !m_loading && !m_hasError
        && !m_query.isEmpty() && (m_currentPage == 0 || m_currentPage < m_totalPages);
}

void SeriesModel::fetchMore(const QModelIndex &parent)
{
    if (!canFetchMore(parent))
        return;
    m_loading = true;
    m_pendingRequestId = m_tmdb->searchSeries(m_query, m_currentPage + 1);
    if (m_pendingRequestId == Tmdb::InvalidRequestId) {
        m_loading = false;
        m_hasError = true;
        m_lastError = {Tmdb::ErrorCode::InvalidRequest, 0,
                       tr("The TV-series search request was not accepted.")};
        Q_EMIT loadingChanged(false);
        Q_EMIT requestFailed(Tmdb::InvalidRequestId, m_lastError);
        return;
    }
    Q_EMIT loadingChanged(true);
}

void SeriesModel::onSeriesFetched(Tmdb::RequestId requestId, SeriesSearchResult result)
{
    if (requestId == Tmdb::InvalidRequestId || requestId != m_pendingRequestId)
        return;
    if (result.page != m_currentPage + 1 || result.totalPages < 0 || result.totalResults < 0
        || (result.totalPages == 0 && (result.page != 1 || !result.items.isEmpty()))
        || (result.totalPages > 0 && result.page > result.totalPages)
        || result.items.size() > std::numeric_limits<int>::max() - m_series.size()) {
        onRequestFailed(requestId, {Tmdb::ErrorCode::InvalidResponse, 0,
                                    tr("Invalid TV-series search pagination response.")});
        return;
    }

    m_resetting = true;
    const int firstRow = int(m_series.size());
    if (!result.items.isEmpty())
        beginInsertRows({}, firstRow, firstRow + int(result.items.size()) - 1);
    m_series.append(result.items);
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

void SeriesModel::onRequestFailed(Tmdb::RequestId requestId, Tmdb::Error error)
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

void SeriesModel::onRequestStateChanged(Tmdb::RequestId requestId, Tmdb::State state)
{
    if (requestId == Tmdb::InvalidRequestId || requestId != m_pendingRequestId)
        return;
    if (state != Tmdb::State::Canceled)
        return;
    m_pendingRequestId = Tmdb::InvalidRequestId;
    m_loading = false;
    Q_EMIT loadingChanged(false);
}

} // namespace Streamable
