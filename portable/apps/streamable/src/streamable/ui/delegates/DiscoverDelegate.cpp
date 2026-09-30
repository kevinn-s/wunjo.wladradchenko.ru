/*
    SPDX-FileCopyrightText: 2026 Streamable Contributors
    SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

/**
 * @file DiscoverDelegate.cpp
 * @brief Implements rendering for movie and TV-series discovery result cards.
 */

#include "DiscoverDelegate.h"

#include "../../models/FilmModel.h"
#include "../../models/SeriesModel.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QDate>
#include <QFontMetrics>
#include <QPainter>
#include <QPainterPath>
#include <QStyle>
#include <QStyleOptionViewItem>
#include <QtMath>

#include <cmath>

namespace Streamable {

static constexpr int kTagFontPixelSize = 20;
static constexpr qreal kCardWidthRatio = 0.56;
static constexpr qreal kPosterWidthRatio = 0.355;

static QString displayDate(const QString &date)
{
    const QDate parsedDate = QDate::fromString(date, QStringLiteral("yyyy-MM-dd"));
    return parsedDate.isValid() ? parsedDate.toString(QStringLiteral("MMM d, yyyy")) : date;
}

static void drawStar(QPainter *painter, const QPointF &center, qreal radius, const QColor &color)
{
    QPolygonF points;
    for (int i = 0; i < 10; ++i) {
        const qreal angle = -1.5707963267948966 + i * 0.6283185307179586;
        const qreal pointRadius = (i % 2 == 0) ? radius : radius * 0.45;
        points.append(center + QPointF(std::cos(angle) * pointRadius,
                                       std::sin(angle) * pointRadius));
    }
    painter->setPen(Qt::NoPen);
    painter->setBrush(color);
    painter->drawPolygon(points);
}

DiscoverDelegate::DiscoverDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void DiscoverDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                              const QModelIndex &index) const
{
    if (!painter || !index.isValid())
        return;

    if (const auto *filmModel = qobject_cast<const FilmModel *>(index.model())) {
        const Film &film = filmModel->FilmAt(index.row());
        paintCard(painter, option, index, film.m_title, film.m_date, film.m_score,
                  film.m_tags, film.m_posterPath);
        return;
    }
    if (const auto *seriesModel = qobject_cast<const SeriesModel *>(index.model())) {
        const Series &series = seriesModel->SeriesAt(index.row());
        paintCard(painter, option, index, series.m_title, series.m_date, series.m_score,
                  series.m_tags, series.m_posterPath);
        return;
    }

    QStyledItemDelegate::paint(painter, option, index);
}

QSize DiscoverDelegate::sizeHint(const QStyleOptionViewItem &option,
                                 const QModelIndex &index) const
{
    if (index.isValid() && !qobject_cast<const FilmModel *>(index.model())
        && !qobject_cast<const SeriesModel *>(index.model()))
        return QStyledItemDelegate::sizeHint(option, index);

    QStyleOptionViewItem styleOption(option);
    initStyleOption(&styleOption, index);
    const int width = option.rect.width() > 0 ? option.rect.width()
                                               : option.widget ? option.widget->width() : 640;
    const int fontHeight = QFontMetrics(styleOption.font).height();
    return {width, qBound(fontHeight * 10, qRound(width * kCardWidthRatio), fontHeight * 20)};
}

void DiscoverDelegate::setPosterImage(const QString &posterPath, const QImage &image)
{
    if (posterPath.isEmpty() || image.isNull())
        return;
    m_posterImages.insert(posterPath, image);
    if (auto *view = qobject_cast<QAbstractItemView *>(parent()))
        view->viewport()->update();
}

void DiscoverDelegate::clearPosterImage(const QString &posterPath)
{
    if (!m_posterImages.remove(posterPath))
        return;
    if (auto *view = qobject_cast<QAbstractItemView *>(parent()))
        view->viewport()->update();
}

void DiscoverDelegate::paintCard(QPainter *painter, const QStyleOptionViewItem &option,
                                 const QModelIndex &index,
                                 const QString &title, const QString &date, double score,
                                 const QStringList &tags, const QString &posterPath) const
{
    QStyleOptionViewItem styleOption(option);
    initStyleOption(&styleOption, index);
    styleOption.text.clear();
    styleOption.icon = {};
    styleOption.features &= ~QStyleOptionViewItem::HasDisplay;
    QStyle *style = styleOption.widget ? styleOption.widget->style() : QApplication::style();

    painter->save();
    style->drawControl(QStyle::CE_ItemViewItem, &styleOption, painter, styleOption.widget);

    const int lineHeight = QFontMetrics(styleOption.font).height();
    const int margin = qMax(8, lineHeight / 2);
    const QRect contentRect = option.rect.adjusted(margin, margin / 2, -margin, -margin / 2);
    if (contentRect.width() <= 0 || contentRect.height() <= 0) {
        painter->restore();
        return;
    }

    const int posterWidth = qRound(contentRect.width() * kPosterWidthRatio);
    const QRect posterRect(contentRect.topLeft(), QSize(posterWidth, contentRect.height()));
    const int gap = qMax(margin, lineHeight);
    const QRect detailRect(contentRect.left() + posterWidth + gap, contentRect.top(),
                           contentRect.width() - posterWidth - gap, contentRect.height());
    if (detailRect.width() <= 0) {
        painter->restore();
        return;
    }

    paintPoster(painter, posterRect, posterPath, styleOption.palette);
    painter->setClipRect(detailRect);

    QFont titleFont = styleOption.font;
    titleFont.setFamily(QStringLiteral("Plus Jakarta Sans"));
    titleFont.setWeight(QFont::Bold);
    titleFont.setPixelSize(qMax(lineHeight + 2, qRound(lineHeight * 1.45)));
    painter->setFont(titleFont);
    painter->setPen(styleOption.state & QStyle::State_Selected
                        ? styleOption.palette.color(QPalette::HighlightedText)
                        : styleOption.palette.color(QPalette::Text));
    const QFontMetrics titleMetrics(titleFont);
    const QRect titleRect(detailRect.left(), detailRect.top(), detailRect.width(),
                          titleMetrics.height() + lineHeight / 3);
    painter->drawText(titleRect, Qt::AlignLeft | Qt::AlignVCenter,
                      titleMetrics.elidedText(title, Qt::ElideRight, titleRect.width()));

    QFont bodyFont = styleOption.font;
    bodyFont.setFamily(QStringLiteral("Plus Jakarta Sans"));
    bodyFont.setWeight(QFont::Medium);
    painter->setFont(bodyFont);
    painter->setPen(styleOption.state & QStyle::State_Selected
                        ? styleOption.palette.color(QPalette::HighlightedText)
                        : styleOption.palette.color(QPalette::Text));
    const int dateY = titleRect.bottom() + lineHeight / 3;
    const int badgeWidth = score > 0.0 ? qMax(lineHeight * 5, QFontMetrics(bodyFont).horizontalAdvance(QStringLiteral("100%")) + lineHeight * 2)
                                       : 0;
    const QRect badgeRect(detailRect.right() - badgeWidth + 1, dateY,
                          badgeWidth, lineHeight * 2);
    const QRect dateRect(detailRect.left(), dateY, detailRect.width() - badgeWidth - lineHeight / 2,
                         lineHeight * 2);
    const QString formattedDate = displayDate(date);
    painter->drawText(dateRect, Qt::AlignLeft | Qt::AlignVCenter,
                      QFontMetrics(bodyFont).elidedText(formattedDate, Qt::ElideRight, dateRect.width()));
    if (badgeWidth > 0)
        paintRating(painter, badgeRect, score, styleOption.palette);

    const int tagsY = dateRect.bottom() + lineHeight / 2;
    paintTags(painter, QRect(detailRect.left(), tagsY, detailRect.width(),
                             detailRect.bottom() - tagsY + 1), tags, styleOption.font,
              styleOption.palette);
    painter->restore();
}

void DiscoverDelegate::paintPoster(QPainter *painter, const QRect &rect,
                                   const QString &posterPath, const QPalette &palette) const
{
    const qreal radius = qMax(4.0, rect.width() * 0.035);
    QPainterPath clipPath;
    clipPath.addRoundedRect(rect, radius, radius);
    painter->save();
    painter->setClipPath(clipPath);
    const auto image = m_posterImages.constFind(posterPath);
    if (image != m_posterImages.cend()) {
        const QSize scaledSize = image->size().scaled(rect.size(), Qt::KeepAspectRatioByExpanding);
        const QImage scaledImage = image->scaled(scaledSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        const QRect sourceRect((scaledImage.width() - rect.width()) / 2,
                               (scaledImage.height() - rect.height()) / 2,
                               rect.width(), rect.height());
        painter->drawImage(rect, scaledImage, sourceRect);
    } else {
        painter->fillRect(rect, palette.color(QPalette::AlternateBase));
    }
    painter->restore();
    painter->setPen(palette.color(QPalette::Mid));
    painter->setBrush(Qt::NoBrush);
    painter->drawRoundedRect(rect.adjusted(0, 0, -1, -1), radius, radius);
}

void DiscoverDelegate::paintRating(QPainter *painter, const QRect &rect, double score,
                                   const QPalette &palette) const
{
    if (!qIsFinite(score) || score <= 0.0 || rect.isEmpty())
        return;
    const QColor badgeColor = palette.color(QPalette::Dark);
    const QColor textColor = palette.color(QPalette::BrightText);
    const int radius = rect.height() / 3;
    painter->save();
    painter->setPen(Qt::NoPen);
    painter->setBrush(badgeColor);
    painter->drawRoundedRect(rect.adjusted(0, 1, -1, -1), radius, radius);
    const int iconSize = qMin(rect.height() / 2, rect.width() / 5);
    drawStar(painter, QPointF(rect.left() + rect.height() / 2.0,
                              rect.center().y()), iconSize / 2.0, textColor);
    QFont font = painter->font();
    font.setFamily(QStringLiteral("Plus Jakarta Sans"));
    font.setWeight(QFont::DemiBold);
    painter->setFont(font);
    painter->setPen(textColor);
    const QString rating = QString::number(qRound(qBound(0.0, score * 10.0, 100.0)))
        + QLatin1Char('%');
    const QRect textRect(rect.left() + rect.height(), rect.top(),
                         rect.width() - rect.height(), rect.height());
    painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, rating);
    painter->restore();
}

void DiscoverDelegate::paintTags(QPainter *painter, const QRect &rect,
                                 const QStringList &tags, const QFont &baseFont,
                                 const QPalette &palette) const
{
    if (rect.isEmpty() || tags.isEmpty())
        return;
    QFont font = baseFont;
    font.setFamily(QStringLiteral("Plus Jakarta Sans"));
    font.setWeight(QFont::DemiBold);
    font.setPixelSize(kTagFontPixelSize);
    const QFontMetrics metrics(font);
    const int horizontalPadding = metrics.height() / 2;
    const int verticalPadding = metrics.height() / 5;
    const int pillHeight = metrics.height() + verticalPadding * 2;
    const int spacing = qMax(6, metrics.height() / 3);
    const QColor pillColor = palette.color(QPalette::Link);
    const qreal luminance = 0.2126 * qPow(pillColor.redF(), 2.2)
        + 0.7152 * qPow(pillColor.greenF(), 2.2) + 0.0722 * qPow(pillColor.blueF(), 2.2);
    const QColor textColor = luminance > 0.179 ? QColor(Qt::black) : QColor(Qt::white);
    int x = rect.left();
    int y = rect.top();

    painter->save();
    painter->setClipRect(rect);
    painter->setFont(font);
    for (const QString &tag : tags) {
        int pillWidth = metrics.horizontalAdvance(tag) + horizontalPadding * 2;
        if (pillWidth > rect.width())
            pillWidth = rect.width();
        if (x > rect.left() && x + pillWidth > rect.right() + 1) {
            x = rect.left();
            y += pillHeight + spacing;
        }
        if (y + pillHeight > rect.bottom() + 1)
            break;
        const QRect pillRect(x, y, pillWidth, pillHeight);
        painter->setPen(Qt::NoPen);
        painter->setBrush(pillColor);
        painter->drawRoundedRect(pillRect, pillHeight / 2, pillHeight / 2);
        painter->setPen(textColor);
        painter->drawText(pillRect.adjusted(horizontalPadding, 0, -horizontalPadding, 0),
                          Qt::AlignCenter,
                          metrics.elidedText(tag, Qt::ElideRight,
                                             pillRect.width() - horizontalPadding * 2));
        x += pillWidth + spacing;
    }
    painter->restore();
}

} // namespace Streamable
