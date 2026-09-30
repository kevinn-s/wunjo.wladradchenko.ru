/*
    SPDX-FileCopyrightText: 2026 Streamable Contributors
    SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

/**
 * @file EpisodeModel.cpp
 * @brief Implements the Qt item model for episodes of the active TV season.
 */

#include "EpisodeModel.h"

#include <QDebug>
#include <QVariant>

#include <utility>

namespace Streamable {

EpisodeModel::EpisodeModel(Tmdb *tmdb, QObject *parent)
    : QAbstractListModel(parent)
    , m_tmdb(tmdb)
{
    if (!m_tmdb)
        return;

    connect(m_tmdb, &Tmdb::seasonFetched, this, &EpisodeModel::onSeasonFetched,
            Qt::QueuedConnection);
    connect(m_tmdb, &Tmdb::requestFailed, this, &EpisodeModel::onRequestFailed,
            Qt::QueuedConnection);
    connect(m_tmdb, &Tmdb::requestStateChanged, this, &EpisodeModel::onRequestStateChanged,
            Qt::QueuedConnection);
    connect(m_tmdb, &QObject::destroyed, this, [this] {
        m_tmdb = nullptr;
        m_pendingRequestId = Tmdb::InvalidRequestId;
        if (std::exchange(m_loading, false))
            Q_EMIT loadingChanged(false);
    });
}

EpisodeModel::~EpisodeModel()
{
    if (m_tmdb && m_pendingRequestId != Tmdb::InvalidRequestId) {
        const Tmdb::RequestId requestId = std::exchange(m_pendingRequestId, Tmdb::InvalidRequestId);
        disconnect(m_tmdb, nullptr, this, nullptr);
        m_tmdb->cancelRequest(requestId);
    }
}

int EpisodeModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_episodes.size());
}

QVariant EpisodeModel::data(const QModelIndex &, int) const
{
    return {};
}

void EpisodeModel::setSeason(int seriesId, int seasonNumber)
{
    if (seriesId == m_seriesId && seasonNumber == m_seasonNumber)
        return;
    resetContext(seriesId, seasonNumber);
}

void EpisodeModel::refresh()
{
    if (m_seriesId >= 0 && m_seasonNumber >= 0)
        resetContext(m_seriesId, m_seasonNumber);
}

void EpisodeModel::clear()
{
    if (m_seriesId >= 0 || m_seasonNumber >= 0 || !m_episodes.isEmpty()
        || m_pendingRequestId != Tmdb::InvalidRequestId)
        resetContext(-1, -1);
}

void EpisodeModel::resetContext(int seriesId, int seasonNumber)
{
    const bool contextChanged = seriesId != m_seriesId || seasonNumber != m_seasonNumber;
    const Tmdb::RequestId previousRequest = std::exchange(m_pendingRequestId, Tmdb::InvalidRequestId);
    if (m_tmdb && previousRequest != Tmdb::InvalidRequestId)
        m_tmdb->cancelRequest(previousRequest);

    beginResetModel();
    m_episodes.clear();
    m_seriesId = seriesId;
    m_seasonNumber = seasonNumber;
    m_lastError = {};
    const bool wasLoading = std::exchange(m_loading, false);
    endResetModel();
    if (contextChanged)
        Q_EMIT seasonChanged(m_seriesId, m_seasonNumber);
    if (wasLoading)
        Q_EMIT loadingChanged(false);
    if (m_tmdb && m_seriesId >= 0 && m_seasonNumber >= 0) {
        m_loading = true;
        m_pendingRequestId = m_tmdb->fetchSeason(m_seriesId, m_seasonNumber);
        if (m_pendingRequestId == Tmdb::InvalidRequestId) {
            m_loading = false;
            m_lastError = {Tmdb::ErrorCode::InvalidRequest, 0,
                           tr("The season request was not accepted.")};
            Q_EMIT loadingChanged(false);
            Q_EMIT requestFailed(Tmdb::InvalidRequestId, m_lastError);
            return;
        }
        Q_EMIT loadingChanged(true);
    }
}

int EpisodeModel::seriesId() const { return m_seriesId; }
int EpisodeModel::seasonNumber() const { return m_seasonNumber; }
bool EpisodeModel::isLoading() const { return m_loading; }
Tmdb::Error EpisodeModel::lastError() const { return m_lastError; }

const Episode &EpisodeModel::EpisodeAt(int row) const
{
    if (row < 0 || row >= m_episodes.size()) {
        qWarning("EpisodeModel::EpisodeAt: invalid row %d", row);
        static const Episode empty;
        return empty;
    }
    return m_episodes.at(row);
}

void EpisodeModel::onSeasonFetched(Tmdb::RequestId requestId, Season season)
{
    if (requestId == Tmdb::InvalidRequestId || requestId != m_pendingRequestId)
        return;
    if (season.m_number != m_seasonNumber) {
        onRequestFailed(requestId, {Tmdb::ErrorCode::InvalidResponse, 0,
                                    tr("TMDB returned episodes for an unexpected season.")});
        return;
    }

    QVector<Episode> fetchedEpisodes;
    fetchedEpisodes.reserve(int(season.m_episodes.size()));
    for (const Episode &episode : season.m_episodes)
        fetchedEpisodes.append(episode);
    if (!fetchedEpisodes.isEmpty())
        beginInsertRows({}, 0, int(fetchedEpisodes.size()) - 1);
    m_episodes = std::move(fetchedEpisodes);
    if (!m_episodes.isEmpty())
        endInsertRows();
    m_pendingRequestId = Tmdb::InvalidRequestId;
    m_loading = false;
    Q_EMIT loadingChanged(false);
}

void EpisodeModel::onRequestFailed(Tmdb::RequestId requestId, Tmdb::Error error)
{
    if (requestId == Tmdb::InvalidRequestId || requestId != m_pendingRequestId)
        return;
    m_pendingRequestId = Tmdb::InvalidRequestId;
    m_loading = false;
    m_lastError = error;
    Q_EMIT loadingChanged(false);
    Q_EMIT requestFailed(requestId, error);
}

void EpisodeModel::onRequestStateChanged(Tmdb::RequestId requestId, Tmdb::State state)
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
