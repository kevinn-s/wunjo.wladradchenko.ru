/*
    SPDX-FileCopyrightText: 2026 Streamable Contributors
    SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include <QDateTime>
#include <QJsonObject>
#include <QMetaType>
#include <QObject>
#include <QString>

#include <QtCore/qmath.h>

namespace Streamable {

// Single episode value type. Nullable numerics use sentinels so the type
// stays Q_GADGET/Q_PROPERTY friendly: duration < 0 means unknown,
// qIsNaN(score) means unrated.
struct Episode
{
    Q_GADGET
    Q_PROPERTY(int id MEMBER m_id)
    Q_PROPERTY(QString data MEMBER m_data)
    Q_PROPERTY(QString apiName MEMBER m_apiName)
    Q_PROPERTY(QString title MEMBER m_title)
    Q_PROPERTY(int season MEMBER m_season)
    Q_PROPERTY(int episode MEMBER m_episode)
    Q_PROPERTY(QString date MEMBER m_date)
    Q_PROPERTY(QString description MEMBER m_description)
    Q_PROPERTY(QString subtitle MEMBER m_subtitle)
    Q_PROPERTY(int duration MEMBER m_duration)
    Q_PROPERTY(double score MEMBER m_score)
    Q_PROPERTY(QString posterPath MEMBER m_posterPath)

public:
    int m_id = -1;
    QString m_data; // null: provider-specific payload/URL absent
    QString m_apiName; // null: target provider key absent
    QString m_title;
    int m_season = 0;
    int m_episode = 0;
    QString m_date; // null: unknown; ISO 8601 (YYYY-MM-DD) when set
    QString m_description; // null when absent
    QString m_subtitle; // null when absent
    int m_duration = -1; // nullable, minutes; -1 = unknown
    double m_score = qQNaN(); // nullable; NaN = unrated
    QString m_posterPath; // null when absent

    bool isValid() const { return m_id >= 0; }
    bool hasDuration() const { return m_duration >= 0; }
    bool hasScore() const { return !qIsNaN(m_score); }

    QDateTime dateAsDateTime() const
    {
        if (m_date.isEmpty())
            return {};
        QDateTime dt = QDateTime::fromString(m_date, Qt::ISODate);
        if (!dt.isValid())
            dt = QDateTime::fromString(m_date, QStringLiteral("yyyy-MM-dd"));
        return dt;
    }

    QJsonObject toJson() const
    {
        QJsonObject obj;
        obj.insert(QStringLiteral("id"), m_id);
        if (!m_data.isNull())
            obj.insert(QStringLiteral("data"), m_data);
        if (!m_apiName.isNull())
            obj.insert(QStringLiteral("apiName"), m_apiName);
        obj.insert(QStringLiteral("title"), m_title);
        obj.insert(QStringLiteral("season"), m_season);
        obj.insert(QStringLiteral("episode"), m_episode);
        if (!m_date.isNull())
            obj.insert(QStringLiteral("date"), m_date);
        if (!m_description.isNull())
            obj.insert(QStringLiteral("description"), m_description);
        if (!m_subtitle.isNull())
            obj.insert(QStringLiteral("subtitle"), m_subtitle);
        if (hasDuration())
            obj.insert(QStringLiteral("duration"), m_duration);
        if (hasScore())
            obj.insert(QStringLiteral("score"), m_score);
        if (!m_posterPath.isNull())
            obj.insert(QStringLiteral("poster_path"), m_posterPath);
        return obj;
    }

    static Episode fromJson(const QJsonObject &obj)
    {
        Episode e;
        e.m_id = obj.value(QStringLiteral("id")).toInt(-1);
        if (obj.contains(QStringLiteral("data")) && !obj.value(QStringLiteral("data")).isNull())
            e.m_data = obj.value(QStringLiteral("data")).toString();
        if (obj.contains(QStringLiteral("apiName")) && !obj.value(QStringLiteral("apiName")).isNull())
            e.m_apiName = obj.value(QStringLiteral("apiName")).toString();
        e.m_title = obj.value(QStringLiteral("title")).toString();
        e.m_season = obj.value(QStringLiteral("season")).toInt(0);
        e.m_episode = obj.value(QStringLiteral("episode")).toInt(0);
        if (obj.contains(QStringLiteral("date")) && !obj.value(QStringLiteral("date")).isNull())
            e.m_date = obj.value(QStringLiteral("date")).toString();
        if (obj.contains(QStringLiteral("description")) && !obj.value(QStringLiteral("description")).isNull())
            e.m_description = obj.value(QStringLiteral("description")).toString();
        if (obj.contains(QStringLiteral("subtitle")) && !obj.value(QStringLiteral("subtitle")).isNull())
            e.m_subtitle = obj.value(QStringLiteral("subtitle")).toString();
        if (obj.contains(QStringLiteral("duration")) && !obj.value(QStringLiteral("duration")).isNull())
            e.m_duration = obj.value(QStringLiteral("duration")).toInt(-1);
        if (obj.contains(QStringLiteral("score")) && !obj.value(QStringLiteral("score")).isNull())
            e.m_score = obj.value(QStringLiteral("score")).toDouble(qQNaN());
        if (obj.contains(QStringLiteral("poster_path")) && !obj.value(QStringLiteral("poster_path")).isNull())
            e.m_posterPath = obj.value(QStringLiteral("poster_path")).toString();
        return e;
    }

    bool operator==(const Episode &other) const
    {
        const bool scoresEqual = (qIsNaN(m_score) && qIsNaN(other.m_score)) || m_score == other.m_score;
        return m_id == other.m_id && m_data == other.m_data && m_apiName == other.m_apiName
            && m_title == other.m_title && m_season == other.m_season && m_episode == other.m_episode
            && m_date == other.m_date && m_description == other.m_description && m_subtitle == other.m_subtitle
            && m_duration == other.m_duration && scoresEqual && m_posterPath == other.m_posterPath;
    }
    bool operator!=(const Episode &other) const { return !(*this == other); }
};

} // namespace Streamable

Q_DECLARE_METATYPE(Streamable::Episode)
