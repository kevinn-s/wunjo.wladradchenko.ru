/*
    SPDX-FileCopyrightText: 2026 Streamable Contributors
    SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

/**
 * @file FilmModel.h
 * @brief Declares the paginated Qt item model for TMDB movie search results.
 */

#include "../Tmdb.h"
#include "Film.h"

#include <QAbstractListModel>
#include <QVector>

namespace Streamable {

// Paginated model over TMDB movie search results.
//
// Follows the KDE Discover ReviewsModel separation: FilmModel owns the
// committed query, loaded rows, and pagination state (last loaded page,
// totals, fetch gate), while Tmdb owns networking, parsing, and per-request
// identity. Tmdb never mutates this model's row container; the model only
// appends rows for responses whose RequestId matches its outstanding request.
class FilmModel final : public QAbstractListModel
{
    Q_OBJECT

public:
    /**
     * @brief Creates the model and wires it to the shared Tmdb service.
     * @param tmdb Non-owning service dependency; must outlive the model.
     *        Typically owned by the Streamable root and shared across models.
     * @param parent QObject owner; DiscoverView will take ownership later.
     */
    explicit FilmModel(Tmdb *tmdb, QObject *parent = nullptr);
    ~FilmModel() override;

    FilmModel(const FilmModel &) = delete;
    FilmModel &operator=(const FilmModel &) = delete;

    // QAbstractListModel interface. Row data is exposed through FilmAt();
    // no custom roles are defined.
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool canFetchMore(const QModelIndex &parent) const override;
    void fetchMore(const QModelIndex &parent) override;

    /**
     * @brief Commits a new search query, discards rows, and loads page 1.
     * @param query Search text submitted to Tmdb. Empty query clears the model.
     */
    void setQuery(const QString &query);

    /** @brief Reloads page 1, including retry after a failure stops automatic fetching. */
    void refresh();

    /** @brief Discards rows and pagination state; cancels outstanding request. */
    void clear();

    /** @brief Committed query backing the current rows. */
    QString query() const;

    /** @brief Last successfully loaded page; 0 when nothing loaded yet. */
    int currentPage() const;

    /** @brief Total pages reported by Tmdb for the committed query. */
    int totalPages() const;

    /** @brief Total results reported by Tmdb for the committed query. */
    int totalResults() const;

    /** @brief True while a page request initiated by this model is outstanding. */
    bool isLoading() const;

    /** @brief Last service error for the committed query, if any. */
    Tmdb::Error lastError() const;

    /**
     * @brief Returns the Film stored at the given row.
     * @param row Model row in [0, rowCount()).
     * @return Reference into the model's row container; invalidated by insertion,
     *         reset, removal, or destruction. Delegates use index.row() rather
     *         than requesting properties through index.data().
     *         An invalid row returns an immutable empty Film and logs a warning.
     */
    const Film &FilmAt(int row) const;

Q_SIGNALS:
    /** @brief Emitted when the committed query changes via setQuery/clear. */
    void queryChanged(const QString &query);

    /** @brief Emitted when loading starts or finishes for this model. */
    void loadingChanged(bool loading);

    /** @brief Emitted when the outstanding request for this model fails. */
    void requestFailed(Tmdb::RequestId requestId, Tmdb::Error error);

private Q_SLOTS:
    void onFilmsFetched(Tmdb::RequestId requestId, FilmSearchResult result);
    void onRequestFailed(Tmdb::RequestId requestId, Tmdb::Error error);
    void onRequestStateChanged(Tmdb::RequestId requestId, Tmdb::State state);

private:
    void resetSearch(const QString &query);
    void fetchNextImage();
    void onImageDownloadFinished(QNetworkReply *reply);

    Tmdb *m_tmdb = nullptr; // Non-owning; owned by the Streamable root object.
    QVector<Film> m_films;
    QSet<QString> m_pendingImages;
    QString m_query;
    int m_currentPage = 0;
    int m_totalPages = 0;
    int m_totalResults = 0;
    bool m_loading = false;
    Tmdb::RequestId m_pendingRequestId = Tmdb::InvalidRequestId;
    Tmdb::Error m_lastError;
    bool m_hasError = false;
    bool m_resetting = false;
    bool m_isDownloadingImage = false;
};

} // namespace Streamable
