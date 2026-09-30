/*
    SPDX-FileCopyrightText: 2026 Streamable Contributors
    SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

/**
 * @file DiscoverView.h
 * @brief Declares the Streamable view for browsing movie and TV-series search
 *        results.
 */

#include "../models/Film.h"
#include "../models/Series.h"

#include <QString>
#include <QWidget>

class QListView;
class QLabel;
class QAbstractItemModel;
class QProgressBar;
class QStackedWidget;

namespace Streamable {

class FilmModel;
class SeriesModel;
class Tmdb;

class DiscoverView final : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief Creates discovery view and its child models.
     * @param tmdb Non-owning service dependency passed to both child models.
     * @param parent QObject owner, normally Streamable root widget.
     */
    explicit DiscoverView(Tmdb *tmdb, QWidget *parent = nullptr);

    DiscoverView(const DiscoverView &) = delete;
    DiscoverView &operator=(const DiscoverView &) = delete;

    /** @brief Starts movie search and displays FilmModel results. */
    void searchFilms(const QString &query);

    /** @brief Starts TV-series search and displays SeriesModel results. */
    void searchSeries(const QString &query);

    /** @brief Clears both result models and detaches active model from view. */
    void clear();

Q_SIGNALS:
    /** @brief Emits activated movie value; QModelIndex stays internal. */
    void filmActivated(const Film &film);

    /** @brief Emits activated TV-series value; QModelIndex stays internal. */
    void seriesActivated(const Series &series);

private:
    void updateModelPresentation(const QAbstractItemModel *model, bool loading,
                                 int rowCount, int currentPage);
    void showError(const QString &message);

    FilmModel *m_filmModel = nullptr;
    SeriesModel *m_seriesModel = nullptr;
    QStackedWidget *m_stateStack = nullptr;
    QListView *m_resultView = nullptr;
    QProgressBar *m_loadingIndicator = nullptr;
    QLabel *m_loadingLabel = nullptr;
    QLabel *m_emptyLabel = nullptr;
    QLabel *m_errorLabel = nullptr;
};

} // namespace Streamable
