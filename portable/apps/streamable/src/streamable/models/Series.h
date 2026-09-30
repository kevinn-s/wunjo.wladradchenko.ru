/*
    SPDX-FileCopyrightText: 2026 Streamable Contributors
    SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include "Episode.h"
#include "Season.h"
#include "misc/TrailerData.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QMetaType>
#include <QObject>
#include <QString>
#include <QStringList>

#include <memory>

namespace Streamable {

// TV-show detail value type.
//
// Cycle safety (per recloudstream/cloudstream LoadResponse design, where
// recommendations is List<SearchResponse> rather than List<LoadResponse>):
// nested recommendations are held as shared_ptr<Series> so a Series never
// contains itself by value, and JSON (de)serialization is depth-limited.
// Populate recommendations shallowly (id/title/poster) unless a deep copy
// is explicitly required.
struct Series
{
    Q_GADGET
    Q_PROPERTY(int id MEMBER m_id)
    Q_PROPERTY(QString title MEMBER m_title)
    Q_PROPERTY(bool adult MEMBER m_adult)
    Q_PROPERTY(bool released MEMBER m_released)
    Q_PROPERTY(QString date MEMBER m_date)
    Q_PROPERTY(int duration MEMBER m_duration)
    Q_PROPERTY(double score MEMBER m_score)
    Q_PROPERTY(QStringList tags MEMBER m_tags)
    Q_PROPERTY(QString backdropPath MEMBER m_backdropPath)
    Q_PROPERTY(QString posterPath MEMBER m_posterPath)

public:
    int m_id = -1;
    QString m_title;
    bool m_adult = false;
    bool m_released = false;
    QString m_date; // null: unknown; ISO 8601 when set
    int m_duration = 0; // average episode duration, minutes
    double m_score = 0.0;
    QStringList m_tags;
    QString m_backdropPath; // null when absent
    QString m_posterPath; // null when absent
    QList<Episode> m_episodes;
    QList<Season> m_seasons;
    QList<std::shared_ptr<Series>> m_recommendations;
    QList<TrailerData> m_trailers;

    bool isValid() const { return m_id >= 0; }

    QDateTime dateAsDateTime() const
    {
        if (m_date.isEmpty())
            return {};
        QDateTime dt = QDateTime::fromString(m_date, Qt::ISODate);
        if (!dt.isValid())
            dt = QDateTime::fromString(m_date, QStringLiteral("yyyy-MM-dd"));
        return dt;
    }

    static constexpr int kMaxRecommendationDepth = 3;

    QJsonObject toJson(int depth = 0) const
    {
        QJsonObject obj;
        obj.insert(QStringLiteral("id"), m_id);
        obj.insert(QStringLiteral("title"), m_title);
        obj.insert(QStringLiteral("adult"), m_adult);
        obj.insert(QStringLiteral("released"), m_released);
        if (!m_date.isNull())
            obj.insert(QStringLiteral("date"), m_date);
        obj.insert(QStringLiteral("duration"), m_duration);
        obj.insert(QStringLiteral("score"), m_score);
        QJsonArray tagArr;
        for (const QString &t : m_tags)
            tagArr.append(t);
        obj.insert(QStringLiteral("tags"), tagArr);
        if (!m_backdropPath.isNull())
            obj.insert(QStringLiteral("backdrop_path"), m_backdropPath);
        if (!m_posterPath.isNull())
            obj.insert(QStringLiteral("poster_path"), m_posterPath);
        QJsonArray epArr;
        for (const Episode &e : m_episodes)
            epArr.append(e.toJson());
        obj.insert(QStringLiteral("episodes"), epArr);
        QJsonArray seasonArr;
        for (const Season &s : m_seasons)
            seasonArr.append(s.toJson());
        obj.insert(QStringLiteral("seasons"), seasonArr);
        if (depth < kMaxRecommendationDepth) {
            QJsonArray recArr;
            for (const auto &rec : m_recommendations) {
                if (rec)
                    recArr.append(rec->toJson(depth + 1));
            }
            obj.insert(QStringLiteral("recommendations"), recArr);
        }
        QJsonArray trailerArr;
        for (const TrailerData &t : m_trailers)
            trailerArr.append(t.toJson());
        obj.insert(QStringLiteral("trailers"), trailerArr);
        return obj;
    }

    static Series fromJson(const QJsonObject &obj, int depth = 0)
    {
        Series s;
        s.m_id = obj.value(QStringLiteral("id")).toInt(-1);
        s.m_title = obj.value(QStringLiteral("title")).toString();
        s.m_adult = obj.value(QStringLiteral("adult")).toBool(false);
        s.m_released = obj.value(QStringLiteral("released")).toBool(false);
        if (obj.contains(QStringLiteral("date")) && !obj.value(QStringLiteral("date")).isNull())
            s.m_date = obj.value(QStringLiteral("date")).toString();
        s.m_duration = obj.value(QStringLiteral("duration")).toInt(0);
        s.m_score = obj.value(QStringLiteral("score")).toDouble(0.0);
        const QJsonArray tagArr = obj.value(QStringLiteral("tags")).toArray();
        for (const QJsonValue &v : tagArr)
            s.m_tags.append(v.toString());
        if (obj.contains(QStringLiteral("backdrop_path")) && !obj.value(QStringLiteral("backdrop_path")).isNull())
            s.m_backdropPath = obj.value(QStringLiteral("backdrop_path")).toString();
        if (obj.contains(QStringLiteral("poster_path")) && !obj.value(QStringLiteral("poster_path")).isNull())
            s.m_posterPath = obj.value(QStringLiteral("poster_path")).toString();
        const QJsonArray epArr = obj.value(QStringLiteral("episodes")).toArray();
        for (const QJsonValue &v : epArr) {
            if (v.isObject())
                s.m_episodes.append(Episode::fromJson(v.toObject()));
        }
        const QJsonArray seasonArr = obj.value(QStringLiteral("seasons")).toArray();
        for (const QJsonValue &v : seasonArr) {
            if (v.isObject())
                s.m_seasons.append(Season::fromJson(v.toObject()));
        }
        if (depth < kMaxRecommendationDepth) {
            const QJsonArray recArr = obj.value(QStringLiteral("recommendations")).toArray();
            for (const QJsonValue &v : recArr) {
                if (v.isObject())
                    s.m_recommendations.append(std::make_shared<Series>(fromJson(v.toObject(), depth + 1)));
            }
        }
        const QJsonArray trailerArr = obj.value(QStringLiteral("trailers")).toArray();
        for (const QJsonValue &v : trailerArr) {
            if (v.isObject())
                s.m_trailers.append(TrailerData::fromJson(v.toObject()));
        }
        return s;
    }

    bool operator==(const Series &other) const
    {
        if (m_id != other.m_id || m_title != other.m_title || m_adult != other.m_adult
            || m_released != other.m_released || m_date != other.m_date || m_duration != other.m_duration
            || m_score != other.m_score || m_tags != other.m_tags || m_backdropPath != other.m_backdropPath
            || m_posterPath != other.m_posterPath || m_episodes != other.m_episodes
            || m_seasons != other.m_seasons || m_trailers != other.m_trailers
            || m_recommendations.size() != other.m_recommendations.size())
            return false;
        for (int i = 0; i < m_recommendations.size(); ++i) {
            const auto &a = m_recommendations.at(i);
            const auto &b = other.m_recommendations.at(i);
            if (bool(a) != bool(b))
                return false;
            if (a && b && !(*a == *b))
                return false;
        }
        return true;
    }
    bool operator!=(const Series &other) const { return !(*this == other); }
};

} // namespace Streamable

Q_DECLARE_METATYPE(Streamable::Series)
