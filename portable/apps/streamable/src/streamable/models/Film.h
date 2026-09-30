/*
    SPDX-FileCopyrightText: 2026 Streamable Contributors
    SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include "Series.h"
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

// Movie detail value type. Depends on Series.h (one direction only:
// Series never includes Film), so no include cycle. Recommendations reuse
// the Series shared_ptr pattern documented on Series.
struct Film
{
    Q_GADGET
    Q_PROPERTY(int id MEMBER m_id)
    Q_PROPERTY(QString title MEMBER m_title)
    Q_PROPERTY(bool adult MEMBER m_adult)
    Q_PROPERTY(bool released MEMBER m_released)
    Q_PROPERTY(QString date MEMBER m_date)
    Q_PROPERTY(QString subtitle MEMBER m_subtitle)
    Q_PROPERTY(int duration MEMBER m_duration)
    Q_PROPERTY(double score MEMBER m_score)
    Q_PROPERTY(QStringList tags MEMBER m_tags)
    Q_PROPERTY(QString backdropPath MEMBER m_backdropPath)
    Q_PROPERTY(QString posterPath MEMBER m_posterPath)
    Q_PROPERTY(QString data MEMBER m_data)
    Q_PROPERTY(QString apiName MEMBER m_apiName)

public:
    int m_id = -1;
    QString m_title;
    bool m_adult = false;
    bool m_released = false;
    QString m_date; // ISO 8601 (YYYY-MM-DD) when set
    QString m_subtitle; // null when absent
    int m_duration = 0; // minutes
    double m_score = 0.0;
    QStringList m_tags;
    QString m_backdropPath; // null when absent
    QString m_posterPath; // null when absent
    QList<std::shared_ptr<Series>> m_recommendations;
    QList<TrailerData> m_trailers;
    QString m_data; // null: provider-specific URL/ID payload absent
    QString m_apiName; // null: target provider identifier absent

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

    QJsonObject toJson() const
    {
        QJsonObject obj;
        obj.insert(QStringLiteral("id"), m_id);
        obj.insert(QStringLiteral("title"), m_title);
        obj.insert(QStringLiteral("adult"), m_adult);
        obj.insert(QStringLiteral("released"), m_released);
        if (!m_date.isNull())
            obj.insert(QStringLiteral("date"), m_date);
        if (!m_subtitle.isNull())
            obj.insert(QStringLiteral("subtitle"), m_subtitle);
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
        QJsonArray recArr;
        for (const auto &rec : m_recommendations) {
            if (rec)
                recArr.append(rec->toJson());
        }
        obj.insert(QStringLiteral("recommendations"), recArr);
        QJsonArray trailerArr;
        for (const TrailerData &t : m_trailers)
            trailerArr.append(t.toJson());
        obj.insert(QStringLiteral("trailers"), trailerArr);
        if (!m_data.isNull())
            obj.insert(QStringLiteral("data"), m_data);
        if (!m_apiName.isNull())
            obj.insert(QStringLiteral("apiName"), m_apiName);
        return obj;
    }

    static Film fromJson(const QJsonObject &obj)
    {
        Film f;
        f.m_id = obj.value(QStringLiteral("id")).toInt(-1);
        f.m_title = obj.value(QStringLiteral("title")).toString();
        f.m_adult = obj.value(QStringLiteral("adult")).toBool(false);
        f.m_released = obj.value(QStringLiteral("released")).toBool(false);
        if (obj.contains(QStringLiteral("date")) && !obj.value(QStringLiteral("date")).isNull())
            f.m_date = obj.value(QStringLiteral("date")).toString();
        if (obj.contains(QStringLiteral("subtitle")) && !obj.value(QStringLiteral("subtitle")).isNull())
            f.m_subtitle = obj.value(QStringLiteral("subtitle")).toString();
        f.m_duration = obj.value(QStringLiteral("duration")).toInt(0);
        f.m_score = obj.value(QStringLiteral("score")).toDouble(0.0);
        const QJsonArray tagArr = obj.value(QStringLiteral("tags")).toArray();
        for (const QJsonValue &v : tagArr)
            f.m_tags.append(v.toString());
        if (obj.contains(QStringLiteral("backdrop_path")) && !obj.value(QStringLiteral("backdrop_path")).isNull())
            f.m_backdropPath = obj.value(QStringLiteral("backdrop_path")).toString();
        if (obj.contains(QStringLiteral("poster_path")) && !obj.value(QStringLiteral("poster_path")).isNull())
            f.m_posterPath = obj.value(QStringLiteral("poster_path")).toString();
        const QJsonArray recArr = obj.value(QStringLiteral("recommendations")).toArray();
        for (const QJsonValue &v : recArr) {
            if (v.isObject())
                f.m_recommendations.append(std::make_shared<Series>(Series::fromJson(v.toObject())));
        }
        const QJsonArray trailerArr = obj.value(QStringLiteral("trailers")).toArray();
        for (const QJsonValue &v : trailerArr) {
            if (v.isObject())
                f.m_trailers.append(TrailerData::fromJson(v.toObject()));
        }
        if (obj.contains(QStringLiteral("data")) && !obj.value(QStringLiteral("data")).isNull())
            f.m_data = obj.value(QStringLiteral("data")).toString();
        if (obj.contains(QStringLiteral("apiName")) && !obj.value(QStringLiteral("apiName")).isNull())
            f.m_apiName = obj.value(QStringLiteral("apiName")).toString();
        return f;
    }

    bool operator==(const Film &other) const
    {
        if (m_id != other.m_id || m_title != other.m_title || m_adult != other.m_adult
            || m_released != other.m_released || m_date != other.m_date || m_subtitle != other.m_subtitle
            || m_duration != other.m_duration || m_score != other.m_score || m_tags != other.m_tags
            || m_backdropPath != other.m_backdropPath || m_posterPath != other.m_posterPath
            || m_trailers != other.m_trailers || m_data != other.m_data || m_apiName != other.m_apiName
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
    bool operator!=(const Film &other) const { return !(*this == other); }
};

} // namespace Streamable

Q_DECLARE_METATYPE(Streamable::Film)
