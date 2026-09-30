/*
    SPDX-FileCopyrightText: 2026 Streamable Contributors
    SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

/**
 * @file BrowserFilter.h
 * @brief Declares the Streamable search and media-type input widget.
 */

#include <QMetaType>
#include <QString>
#include <QWidget>

class QAction;
class QButtonGroup;
class QEvent;
class QLineEdit;
class QToolButton;

namespace Streamable {

class BrowserFilter final : public QWidget
{
    Q_OBJECT

public:
    enum class SearchType {
        Film,
        Series
    };
    Q_ENUM(SearchType)

    explicit BrowserFilter(QWidget *parent = nullptr);

    BrowserFilter(const BrowserFilter &) = delete;
    BrowserFilter &operator=(const BrowserFilter &) = delete;

Q_SIGNALS:
    /** @brief Emitted for non-empty, trimmed search query and selected type. */
    void SearchRequested(const QString &query, Streamable::BrowserFilter::SearchType type);

private Q_SLOTS:
    void SubmitSearch();

protected:
    void changeEvent(QEvent *event) override;

private:
    void updateTypeButtonStyle();

    QLineEdit *m_searchEdit = nullptr;
    QAction *m_searchAction = nullptr;
    QButtonGroup *m_typeButtonGroup = nullptr;
    QToolButton *m_filmButton = nullptr;
    QToolButton *m_seriesButton = nullptr;
};

} // namespace Streamable

Q_DECLARE_METATYPE(Streamable::BrowserFilter::SearchType)
