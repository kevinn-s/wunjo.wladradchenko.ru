/*
    SPDX-FileCopyrightText: 2026 Streamable Contributors
    SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

/**
 * @file EpisodeModel.h
 * @brief Declares the Qt item model for episodes of the active TV season.
 */

#include "../Tmdb.h"
#include "Episode.h"

#include <QAbstractListModel>
#include <QVector>

namespace Streamable {

class EpisodeModel final : public QAbstractListModel
{
    Q_OBJECT

public:
    /**
     * @brief Creates model and connects it to shared Tmdb service.
     * @param tmdb Non-owning dependency; must outlive model.
     * @param parent QObject owner for this context-scoped model.
     */
    explicit EpisodeModel(Tmdb *tmdb, QObject *parent = nullptr);
    ~EpisodeModel() override;

    EpisodeModel(const EpisodeModel &) = delete;
    EpisodeModel &operator=(const EpisodeModel &) = delete;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    /** @brief Changes active season, resets rows, and requests season episodes.
     *  @param seriesId TMDB TV-series identifier.
     *  @param seasonNumber TMDB season number; specials may use zero.
     */
    void setSeason(int seriesId, int seasonNumber);

    /** @brief Reloads episode data for active season. */
    void refresh();

    /** @brief Clears active context, rows, and any outstanding request. */
    void clear();

    /** @brief Active TV-series TMDB ID, or -1 when no season is selected. */
    int seriesId() const;

    /** @brief Active season number, or -1 when no season is selected. */
    int seasonNumber() const;

    /** @brief True while active-season request is outstanding. */
    bool isLoading() const;

    /** @brief Last active-season request error, if any. */
    Tmdb::Error lastError() const;

    /**
     * @brief Returns episode stored at row.
     * @param row Model row in [0, rowCount()).
     * @return Reference into model storage; invalidated by reset, removal, or
     *         destruction. Invalid row returns immutable empty value.
     */
    const Episode &EpisodeAt(int row) const;

Q_SIGNALS:
    /** @brief Emitted when active series/season context changes. */
    void seasonChanged(int seriesId, int seasonNumber);

    /** @brief Emitted when active-season request starts or finishes. */
    void loadingChanged(bool loading);

    /** @brief Emitted when active-season request fails. */
    void requestFailed(Tmdb::RequestId requestId, Tmdb::Error error);

private Q_SLOTS:
    void onSeasonFetched(Tmdb::RequestId requestId, Season season);
    void onRequestFailed(Tmdb::RequestId requestId, Tmdb::Error error);
    void onRequestStateChanged(Tmdb::RequestId requestId, Tmdb::State state);

private:
    void resetContext(int seriesId, int seasonNumber);

    Tmdb *m_tmdb = nullptr; // Non-owning; service owner controls lifetime.
    QVector<Episode> m_episodes;
    int m_seriesId = -1;
    int m_seasonNumber = -1;
    bool m_loading = false;
    Tmdb::RequestId m_pendingRequestId = Tmdb::InvalidRequestId;
    Tmdb::Error m_lastError;
};

} // namespace Streamable
