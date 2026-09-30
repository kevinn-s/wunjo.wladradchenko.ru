/*
    SPDX-FileCopyrightText: 2026 Streamable Contributors
    SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include "Episode.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QMetaType>
#include <QObject>
#include <QString>

#include <QtCore/qmath.h>

namespace Streamable {

// Season value type. Episodes held by value: Episode never refers back to
// Season, so no cycle is possible here.
struct Season
{
    Q_GADGET
    Q_PROPERTY(int id MEMBER m_id)
    Q_PROPERTY(QString title MEMBER m_title)
    Q_PROPERTY(int number MEMBER m_number)
    Q_PROPERTY(QString description MEMBER m_description)
    Q_PROPERTY(QString posterPath MEMBER m_posterPath)
    Q_PROPERTY(double score MEMBER m_score)

public:
    int m_id = -1;
    QString m_title;
    int m_number = 0;
    QString m_description; // null when absent
    QString m_posterPath; // null when absent
    double m_score = qQNaN(); // nullable; NaN = unrated
    QList<Episode> m_episodes;

    bool isValid() const { return m_id >= 0; }
    bool hasScore() const { return !qIsNaN(m_score); }

    QJsonObject toJson() const
    {
        QJsonObject obj;
        obj.insert(QStringLiteral("id"), m_id);
        obj.insert(QStringLiteral("title"), m_title);
        obj.insert(QStringLiteral("number"), m_number);
        if (!m_description.isNull())
            obj.insert(QStringLiteral("description"), m_description);
        if (!m_posterPath.isNull())
            obj.insert(QStringLiteral("poster_path"), m_posterPath);
        if (hasScore())
            obj.insert(QStringLiteral("score"), m_score);
        QJsonArray arr;
        for (const Episode &e : m_episodes)
            arr.append(e.toJson());
        obj.insert(QStringLiteral("episodes"), arr);
        return obj;
    }

    static Season fromJson(const QJsonObject &obj)
    {
        Season s;
        s.m_id = obj.value(QStringLiteral("id")).toInt(-1);
        s.m_title = obj.value(QStringLiteral("title")).toString();
        s.m_number = obj.value(QStringLiteral("number")).toInt(0);
        if (obj.contains(QStringLiteral("description")) && !obj.value(QStringLiteral("description")).isNull())
            s.m_description = obj.value(QStringLiteral("description")).toString();
        if (obj.contains(QStringLiteral("poster_path")) && !obj.value(QStringLiteral("poster_path")).isNull())
            s.m_posterPath = obj.value(QStringLiteral("poster_path")).toString();
        if (obj.contains(QStringLiteral("score")) && !obj.value(QStringLiteral("score")).isNull())
            s.m_score = obj.value(QStringLiteral("score")).toDouble(qQNaN());
        const QJsonArray arr = obj.value(QStringLiteral("episodes")).toArray();
        for (const QJsonValue &v : arr) {
            if (v.isObject())
                s.m_episodes.append(Episode::fromJson(v.toObject()));
        }
        return s;
    }

    bool operator==(const Season &other) const
    {
        const bool scoresEqual = (qIsNaN(m_score) && qIsNaN(other.m_score)) || m_score == other.m_score;
        return m_id == other.m_id && m_title == other.m_title && m_number == other.m_number
            && m_description == other.m_description && m_posterPath == other.m_posterPath
            && scoresEqual && m_episodes == other.m_episodes;
    }
    bool operator!=(const Season &other) const { return !(*this == other); }
};

} // namespace Streamable

Q_DECLARE_METATYPE(Streamable::Season)
