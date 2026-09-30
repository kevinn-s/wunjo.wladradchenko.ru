/*
    SPDX-FileCopyrightText: 2026 Streamable Contributors
    SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

/**
 * @file Streamable.h
 * @brief Declares the top-level Streamable discovery container.
 */

#include <QWidget>

namespace Streamable {

class BrowserFilter;
class DiscoverView;
class Tmdb;

class StreamableWidget final : public QWidget
{
    Q_OBJECT

public:
    explicit StreamableWidget(QWidget *parent = nullptr);

    StreamableWidget(const StreamableWidget &) = delete;
    StreamableWidget &operator=(const StreamableWidget &) = delete;

private:
    Tmdb *m_tmdb = nullptr;
    BrowserFilter *m_browserFilter = nullptr;
    DiscoverView *m_discoverView = nullptr;
};

} // namespace Streamable
