/*
    SPDX-FileCopyrightText: 2026 Streamable Contributors
    SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

/**
 * @file DiscoverDelegate.h
 * @brief Declares the delegate used to render Streamable discovery results.
 */

#include <QHash>
#include <QImage>
#include <QStyledItemDelegate>

namespace Streamable {

class DiscoverDelegate final : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit DiscoverDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override;

    /** @brief Adds or replaces locally available poster image for poster path. */
    void setPosterImage(const QString &posterPath, const QImage &image);

    /** @brief Removes locally cached poster image for poster path. */
    void clearPosterImage(const QString &posterPath);

private:
    void paintCard(QPainter *painter, const QStyleOptionViewItem &option,
                   const QModelIndex &index,
                   const QString &title, const QString &date, double score,
                   const QStringList &tags, const QString &posterPath) const;
    void paintPoster(QPainter *painter, const QRect &rect, const QString &posterPath,
                     const QPalette &palette) const;
    void paintRating(QPainter *painter, const QRect &rect, double score,
                     const QPalette &palette) const;
    void paintTags(QPainter *painter, const QRect &rect, const QStringList &tags,
                   const QFont &baseFont, const QPalette &palette) const;

    QHash<QString, QImage> m_posterImages;
};

} // namespace Streamable
