/*
    SPDX-FileCopyrightText: 2026 Streamable Contributors
    SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include <QJsonObject>
#include <QMap>
#include <QMetaType>
#include <QObject>
#include <QString>
#include <QVariantMap>

namespace Streamable {

// Cloudstream TrailerData equivalent: self-contained trailer reference.
// Mirrors recloudstream/cloudstream LoadResponse.trailers: MutableList<TrailerData>
// with extractorUrl / referer / raw, plus per-request headers.
struct TrailerData
{
    Q_GADGET
    Q_PROPERTY(QString extractorUrl MEMBER m_extractorUrl)
    Q_PROPERTY(QString referer MEMBER m_referer)
    Q_PROPERTY(bool raw MEMBER m_raw)

public:
    QString m_extractorUrl;
    QString m_referer; // null when no referer header is required
    bool m_raw = false; // true: direct stream, false: needs extraction
    QMap<QString, QString> m_headers; // default: empty

    bool isValid() const { return !m_extractorUrl.isEmpty(); }

    // Mirrors ExtractorLink.getAllHeaders(): headers plus referer when set
    // and not already present.
    QMap<QString, QString> allHeaders() const
    {
        QMap<QString, QString> merged = m_headers;
        if (!m_referer.isEmpty() && !merged.contains(QStringLiteral("Referer")))
            merged.insert(QStringLiteral("Referer"), m_referer);
        return merged;
    }

    QJsonObject toJson() const
    {
        QJsonObject obj;
        obj.insert(QStringLiteral("extractorUrl"), m_extractorUrl);
        if (!m_referer.isNull())
            obj.insert(QStringLiteral("referer"), m_referer);
        obj.insert(QStringLiteral("raw"), m_raw);
        QJsonObject h;
        for (auto it = m_headers.constBegin(); it != m_headers.constEnd(); ++it)
            h.insert(it.key(), it.value());
        obj.insert(QStringLiteral("headers"), h);
        return obj;
    }

    static TrailerData fromJson(const QJsonObject &obj)
    {
        TrailerData t;
        t.m_extractorUrl = obj.value(QStringLiteral("extractorUrl")).toString();
        if (obj.contains(QStringLiteral("referer")))
            t.m_referer = obj.value(QStringLiteral("referer")).toString();
        t.m_raw = obj.value(QStringLiteral("raw")).toBool(false);
        const QJsonObject h = obj.value(QStringLiteral("headers")).toObject();
        for (auto it = h.constBegin(); it != h.constEnd(); ++it)
            t.m_headers.insert(it.key(), it.value().toString());
        return t;
    }

    bool operator==(const TrailerData &other) const
    {
        return m_extractorUrl == other.m_extractorUrl && m_referer == other.m_referer
            && m_raw == other.m_raw && m_headers == other.m_headers;
    }
    bool operator!=(const TrailerData &other) const { return !(*this == other); }
};

} // namespace Streamable

Q_DECLARE_METATYPE(Streamable::TrailerData)
