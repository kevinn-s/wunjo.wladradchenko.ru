/*
    SPDX-FileCopyrightText: 2026 Streamable Contributors
    SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

/**
 * @file SeriesModel.h
 * @brief Declares the infinite-scrolling Qt item model for TMDB TV-series
 *        search results.
 */

#include "../Tmdb.h"
#include "Series.h"

#include <QAbstractListModel>
#include <QVector>

namespace Streamable {

// Paginated series model; query, rows, and scrolling state stay model-owned.
// Tmdb performs network and JSON/domain work, and returns results tagged with
// request identities so stale responses can be ignored.
class SeriesModel final : public QAbstractListModel
{
    Q_OBJECT

public:
    /**
     * @brief Creates model and connects it to shared Tmdb service.
     * @param tmdb Non-owning dependency; must outlive model.
     * @param parent QObject owner, typically a view owning this model.
     */
    explicit SeriesModel(Tmdb *tmdb, QObject *parent = nullptr);
    ~SeriesModel() override;

    SeriesModel(const SeriesModel &) = delete;
    SeriesModel &operator=(const SeriesModel &) = delete;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool canFetchMore(const QModelIndex &parent) const override;
    void fetchMore(const QModelIndex &parent) override;

    /** @brief Commits query, clears old results, and starts first page search. */
    void setQuery(const QString &query);

    /** @brief Reloads first page, including retry after a failure. */
    void refresh();

    /** @brief Clears query/results/pagination and cancels any outstanding request. */
    void clear();

    /** @brief Current committed search query. */
    QString query() const;

    /** @brief Last successfully loaded TMDB page, or zero before first success. */
    int currentPage() const;

    /** @brief Total TMDB pages for current query. */
    int totalPages() const;

    /** @brief Total TMDB results for current query. */
    int totalResults() const;

    /** @brief Whether model currently awaits a Tmdb response. */
    bool isLoading() const;

    /** @brief Last error from current search, if any. */
    Tmdb::Error lastError() const;

    /**
     * @brief Returns series stored at row.
     * @param row Model row in [0, rowCount()).
     * @return Reference into model storage; invalidated by insertion, reset,
     *         removal, or destruction. Invalid row returns immutable empty value.
     */
    const Series &SeriesAt(int row) const;

Q_SIGNALS:
    /** @brief Emitted when committed query changes. */
    void queryChanged(const QString &query);

    /** @brief Emitted when loading starts or finishes. */
    void loadingChanged(bool loading);

    /** @brief Emitted when current request fails. */
    void requestFailed(Tmdb::RequestId requestId, Tmdb::Error error);

private Q_SLOTS:
    void onSeriesFetched(Tmdb::RequestId requestId, SeriesSearchResult result);
    void onRequestFailed(Tmdb::RequestId requestId, Tmdb::Error error);
    void onRequestStateChanged(Tmdb::RequestId requestId, Tmdb::State state);

private:
    void resetSearch(const QString &query);

    Tmdb *m_tmdb = nullptr; // Non-owning; owner controls service lifetime.
    QVector<Series> m_series;
    QString m_query;
    int m_currentPage = 0;
    int m_totalPages = 0;
    int m_totalResults = 0;
    bool m_loading = false;
    Tmdb::RequestId m_pendingRequestId = Tmdb::InvalidRequestId;
    Tmdb::Error m_lastError;
    bool m_hasError = false;
    bool m_resetting = false;
};

} // namespace Streamable
