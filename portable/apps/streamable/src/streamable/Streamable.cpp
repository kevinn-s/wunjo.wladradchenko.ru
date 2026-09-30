/*
    SPDX-FileCopyrightText: 2026 Streamable Contributors
    SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

/**
 * @file Streamable.cpp
 * @brief Implements the top-level Streamable discovery container.
 */

#include "Streamable.h"

#include "Tmdb.h"
#include "ui/BrowserFilter.h"
#include "ui/DiscoverView.h"

#include <QVBoxLayout>

namespace Streamable {

StreamableWidget::StreamableWidget(QWidget *parent)
    : QWidget(parent)
    , m_tmdb(new Tmdb(this))
    , m_browserFilter(new BrowserFilter(this))
    , m_discoverView(new DiscoverView(m_tmdb, this))
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_browserFilter);
    layout->addWidget(m_discoverView, 1);

    connect(m_browserFilter, &BrowserFilter::SearchRequested, this,
            [this](const QString &query, BrowserFilter::SearchType type) {
        switch (type) {
        case BrowserFilter::SearchType::Film:
            m_discoverView->searchFilms(query);
            break;
        case BrowserFilter::SearchType::Series:
            m_discoverView->searchSeries(query);
            break;
        }
    });
}

} // namespace Streamable
