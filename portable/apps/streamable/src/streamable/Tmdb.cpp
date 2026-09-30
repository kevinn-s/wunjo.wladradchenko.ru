/*
    SPDX-FileCopyrightText: 2026 Streamable Contributors
    SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

/**
 * @file Tmdb.cpp
 * @brief Implements asynchronous communication with The Movie Database (TMDB)
 *        API for the Streamable feature.
 *
 * Handles authenticated TMDB request/response cycles, validates network and API
 * responses, parses JSON payloads, and maps them into Streamable domain types.
 *
 * Key responsibilities are as follows:
 * 1. API Integration: Constructs and dispatches authenticated TMDB requests.
 * 2. Search Requests: Performs paginated movie and TV-series searches used by
 *    the infinite-scrolling search models.
 * 3. Detail Requests: Fetches film, series, season, episode, and trailer data.
 * 4. Domain Mapping: Converts TMDB JSON payloads into Film, Series, Season,
 *    Episode, and TrailerData objects.
 * 5. Error and Request Handling: Reports completion, failure, cancellation, and
 *    request identity through the service contract declared in Tmdb.h.
 *
 * Tmdb contains no widget, item-view, model-storage, or navigation logic.
 *
 * @see FilmModel
 * @see SeriesModel
 * @see SeasonModel
 * @see EpisodeModel
 */

#include "Tmdb.h"

#include <QCoreApplication>
#include <QDate>
#include <QDir>
#include <QFile>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLoggingCategory>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPointer>
#include <QStandardPaths>
#include <QStringList>
#include <QTimer>
#include <QUrlQuery>

#include <cmath>
#include <functional>
#include <limits>
#include <optional>

namespace Streamable {

Q_LOGGING_CATEGORY(tmdbLog, "wunjo.streamable.tmdb")

static QString accessTokenFromDotEnv(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};

    const QStringList lines = QString::fromUtf8(file.readAll()).split(QLatin1Char('\n'));
    for (QString line : lines) {
        line = line.trimmed();
        if (line.startsWith(QChar(0xfeff)))
            line.remove(0, 1);
        if (line.isEmpty() || line.startsWith(QLatin1Char('#')))
            continue;
        if (line.startsWith(QStringLiteral("export ")))
            line.remove(0, 7);

        const qsizetype separator = line.indexOf(QLatin1Char('='));
        if (separator < 0 || line.left(separator).trimmed() != QStringLiteral("tmdb_access_token"))
            continue;

        QString value = line.mid(separator + 1).trimmed();
        if (value.size() >= 2
            && (value.startsWith(QLatin1Char('\'')) || value.startsWith(QLatin1Char('"')))) {
            const QChar quote = value.front();
            const qsizetype closingQuote = value.indexOf(quote, 1);
            if (closingQuote < 0)
                return {};
            value = value.mid(1, closingQuote - 1);
        } else {
            for (qsizetype i = 0; i < value.size(); ++i) {
                if (value.at(i) == QLatin1Char('#') && i > 0 && value.at(i - 1).isSpace()) {
                    value.truncate(i);
                    break;
                }
            }
            value = value.trimmed();
        }
        return value;
    }
    return {};
}

static QString tmdbAccessToken()
{
    const QString environmentToken = QString::fromUtf8(qgetenv("tmdb_access_token")).trimmed();
    if (!environmentToken.isEmpty())
        return environmentToken;

    QStringList paths;
    paths.append(QDir::current().filePath(QStringLiteral(".env")));
    paths.append(QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral(".env")));
    const QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    if (!configPath.isEmpty())
        paths.append(QDir(configPath).filePath(QStringLiteral(".env")));

    for (const QString &path : paths) {
        const QString token = accessTokenFromDotEnv(path).trimmed();
        if (!token.isEmpty())
            return token;
    }
    return {};
}

static int integer(const QJsonValue &value, int fallback = -1)
{
    const double number = value.toDouble(-1);
    if (!value.isDouble() || number < 0 || number > std::numeric_limits<int>::max()
        || std::floor(number) != number)
        return fallback;
    return int(number);
}

static bool identity(const QJsonObject &object, const char *titleKey)
{
    return integer(object.value("id")) > 0 && object.value(QLatin1String(titleKey)).isString();
}

static bool released(const QString &date)
{
    const QDate releaseDate = QDate::fromString(date, Qt::ISODate);
    return releaseDate.isValid() && releaseDate <= QDate::currentDate();
}

static QStringList genres(const QJsonObject &object)
{
    QStringList result;
    const QJsonArray array = object.value("genres").toArray();
    for (const QJsonValue &genre : array) {
        const QString name = genre.toObject().value("name").toString();
        if (!name.isEmpty())
            result.append(name);
    }
    // Search endpoints carry genre IDs, not names. Preserve those IDs as tags.
    if (array.isEmpty()) {
        const QJsonArray ids = object.value("genre_ids").toArray();
        for (const QJsonValue &id : ids) {
            if (integer(id) > 0)
                result.append(QString::number(integer(id)));
        }
    }
    return result;
}

static std::optional<Episode> parseEpisode(const QJsonObject &object)
{
    if (!identity(object, "name") || integer(object.value("season_number")) < 0
        || integer(object.value("episode_number")) < 0)
        return {};
    Episode episode;
    episode.m_id = integer(object.value("id"));
    episode.m_title = object.value("name").toString();
    episode.m_season = integer(object.value("season_number"));
    episode.m_episode = integer(object.value("episode_number"));
    episode.m_date = object.value("air_date").toString();
    episode.m_description = object.value("overview").toString();
    episode.m_duration = integer(object.value("runtime"));
    episode.m_score = object.value("vote_average").toDouble(qQNaN());
    episode.m_posterPath = object.value("still_path").toString();
    return episode;
}

static std::optional<Season> parseSeason(const QJsonObject &object, bool withEpisodes)
{
    if (!identity(object, "name") || integer(object.value("season_number")) < 0
        || (withEpisodes && !object.value("episodes").isArray()))
        return {};
    Season season;
    season.m_id = integer(object.value("id"));
    season.m_title = object.value("name").toString();
    season.m_number = integer(object.value("season_number"));
    season.m_description = object.value("overview").toString();
    season.m_posterPath = object.value("poster_path").toString();
    season.m_score = object.value("vote_average").toDouble(qQNaN());
    if (withEpisodes) {
        const QJsonArray episodes = object.value("episodes").toArray();
        for (const QJsonValue &value : episodes) {
            const std::optional<Episode> episode = parseEpisode(value.toObject());
            if (!episode || episode->m_season != season.m_number)
                return {};
            season.m_episodes.append(*episode);
        }
    }
    return season;
}

static std::optional<QVector<TrailerData>> parseVideos(const QJsonObject &object)
{
    if (!object.value("results").isArray())
        return {};
    QVector<TrailerData> trailers;
    const QJsonArray array = object.value("results").toArray();
    for (const QJsonValue &value : array) {
        if (!value.isObject())
            return {};
        const QJsonObject video = value.toObject();
        const QString key = video.value("key").toString();
        const QString site = video.value("site").toString();
        const QString type = video.value("type").toString();
        if (key.isEmpty() || site.isEmpty() || type.isEmpty())
            return {};
        if (type != QLatin1String("Trailer") && type != QLatin1String("Teaser"))
            continue;
        QUrl url;
        if (site == QLatin1String("YouTube")) {
            url = QUrl(QStringLiteral("https://www.youtube.com/watch"));
            QUrlQuery query;
            query.addQueryItem(QStringLiteral("v"), key);
            url.setQuery(query);
        } else if (site == QLatin1String("Vimeo")) {
            url = QUrl(QStringLiteral("https://vimeo.com/"));
            url.setPath('/' + key);
        } else {
            continue;
        }
        TrailerData trailer;
        trailer.m_extractorUrl = url.toString(QUrl::FullyEncoded);
        trailers.append(trailer);
    }
    return trailers;
}

static std::optional<Film> parseFilm(const QJsonObject &object)
{
    if (!identity(object, "title"))
        return {};
    Film film;
    film.m_id = integer(object.value("id"));
    film.m_title = object.value("title").toString();
    film.m_adult = object.value("adult").toBool();
    film.m_date = object.value("release_date").toString();
    film.m_released = released(film.m_date);
    film.m_subtitle = object.value("tagline").toString();
    film.m_duration = integer(object.value("runtime"), 0);
    film.m_score = object.value("vote_average").toDouble();
    film.m_tags = genres(object);
    film.m_posterPath = object.value("poster_path").toString();
    film.m_backdropPath = object.value("backdrop_path").toString();
    if (object.contains("videos")) {
        const std::optional<QVector<TrailerData>> videos = parseVideos(object.value("videos").toObject());
        if (!videos)
            return {};
        film.m_trailers = *videos;
    }
    // Film recommendations are movies; the current domain only accepts Series.
    return film;
}

static std::optional<Series> parseSeries(const QJsonObject &object, bool details = false)
{
    if (!identity(object, "name") || (details && !object.value("seasons").isArray()))
        return {};
    Series series;
    series.m_id = integer(object.value("id"));
    series.m_title = object.value("name").toString();
    series.m_adult = object.value("adult").toBool();
    series.m_date = object.value("first_air_date").toString();
    series.m_released = released(series.m_date);
    const QJsonArray runtimes = object.value("episode_run_time").toArray();
    series.m_duration = runtimes.isEmpty() ? 0 : integer(runtimes.first(), 0);
    series.m_score = object.value("vote_average").toDouble();
    series.m_tags = genres(object);
    series.m_posterPath = object.value("poster_path").toString();
    series.m_backdropPath = object.value("backdrop_path").toString();
    if (details) {
        const QJsonArray seasons = object.value("seasons").toArray();
        for (const QJsonValue &value : seasons) {
            const std::optional<Season> season = parseSeason(value.toObject(), false);
            if (!season)
                return {};
            series.m_seasons.append(*season);
        }
        if (object.contains("videos")) {
            const std::optional<QVector<TrailerData>> videos = parseVideos(object.value("videos").toObject());
            if (!videos)
                return {};
            series.m_trailers = *videos;
        }
        if (object.contains("recommendations")) {
            const QJsonValue results = object.value("recommendations").toObject().value("results");
            if (!results.isArray())
                return {};
            const QJsonArray recommendations = results.toArray();
            for (const QJsonValue &value : recommendations) {
                const std::optional<Series> recommendation = parseSeries(value.toObject());
                if (!recommendation)
                    return {};
                series.m_recommendations.append(std::make_shared<Series>(*recommendation));
            }
        }
    }
    return series;
}

template<typename Result, typename Parser>
static std::optional<Result> parseSearch(const QJsonObject &object, int requestedPage, Parser parser)
{
    Result result;
    result.page = integer(object.value("page"));
    result.totalPages = integer(object.value("total_pages"));
    result.totalResults = integer(object.value("total_results"));
    if (result.page != requestedPage || result.totalPages < 0 || result.totalResults < 0
        || !object.value("results").isArray()
        || (result.totalPages > 0 && result.page > result.totalPages))
        return {};
    const QJsonArray items = object.value("results").toArray();
    if (result.totalPages == 0 && (result.page != 1 || !items.isEmpty() || result.totalResults != 0))
        return {};
    for (const QJsonValue &value : items) {
        const auto item = parser(value.toObject());
        if (!item)
            return {};
        result.items.append(*item);
    }
    // TMDB search only permits requests through page 500.
    result.totalPages = qMin(result.totalPages, 500);
    return result;
}

class TmdbPrivate final : public QObject
{
public:
    explicit TmdbPrivate(Tmdb *owner)
        : QObject(owner), q(owner), m_manager(new QNetworkAccessManager(this))
        , m_token(tmdbAccessToken())
    {}

    ~TmdbPrivate() override
    {
        const QList<QNetworkReply *> replies = m_active.values();
        m_active.clear();
        for (QNetworkReply *reply : replies) {
            if (reply) {
                reply->disconnect(this);
                reply->abort();
            }
        }
    }

    using Completion = std::function<bool(const QJsonObject &)>;

    Tmdb::RequestId start(const QString &path, QUrlQuery query, bool valid,
                         std::function<Completion(Tmdb::RequestId)> completion)
    {
        if (m_nextId == std::numeric_limits<Tmdb::RequestId>::max())
            return Tmdb::InvalidRequestId;
        const Tmdb::RequestId id = ++m_nextId;
        m_active.insert(id, nullptr);
        const QString token = m_token;
        query.addQueryItem(QStringLiteral("language"), QStringLiteral("en-US"));
        QUrl url(QStringLiteral("https://api.themoviedb.org/3/") + path);
        url.setQuery(query);
        QTimer::singleShot(0, this, [this, id, url, valid, token, done = completion(id)] {
            if (!m_active.contains(id))
                return;
            QPointer<TmdbPrivate> guard(this);
            Q_EMIT q->requestStateChanged(id, Tmdb::State::Loading);
            if (!guard || !m_active.contains(id))
                return;
            if (!valid) {
                fail(id, {Tmdb::ErrorCode::InvalidRequest, 0, tr("Invalid TMDB request parameters.")});
                return;
            }
            if (token.isEmpty() || token.contains('\r') || token.contains('\n')) {
                fail(id, {Tmdb::ErrorCode::Authentication, 0, tr("Missing or invalid TMDB access token.")});
                return;
            }
            QNetworkRequest request(url);
            request.setRawHeader("Accept", "application/json");
            request.setRawHeader("Authorization", "Bearer " + token.toUtf8());
            request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
            request.setTransferTimeout(30000);
            QNetworkReply *reply = m_manager->get(request);
            m_active[id] = reply;
            connect(reply, &QNetworkReply::finished, this, [this, id, reply, done] {
                if (m_active.contains(id))
                    finish(id, reply, done);
            }, Qt::QueuedConnection);
        });
        return id;
    }

    bool cancel(Tmdb::RequestId id)
    {
        if (!m_active.contains(id))
            return false;
        QNetworkReply *reply = m_active.take(id);
        if (reply) {
            reply->disconnect(this);
            reply->abort();
            reply->deleteLater();
        }
        Q_EMIT q->requestStateChanged(id, Tmdb::State::Canceled);
        return true;
    }

    void fail(Tmdb::RequestId id, const Tmdb::Error &error)
    {
        if (!m_active.remove(id))
            return;
        qCWarning(tmdbLog) << "Request" << id << "failed; category" << int(error.code)
                          << "HTTP" << error.httpStatus;
        QPointer<Tmdb> owner(q);
        Q_EMIT owner->requestStateChanged(id, Tmdb::State::Failed);
        if (owner)
            Q_EMIT owner->requestFailed(id, error);
    }

    /** Terminal transition precedes the payload; cancellation cannot undo a committed result. */
    template<typename Result, typename Signal>
    void succeed(Tmdb::RequestId id, Result result, Signal signal)
    {
        m_active.remove(id);
        QPointer<Tmdb> owner(q);
        Q_EMIT owner->requestStateChanged(id, Tmdb::State::Ready);
        if (owner)
            (owner.data()->*signal)(id, result);
    }

    void finish(Tmdb::RequestId id, QNetworkReply *reply, const Completion &done)
    {
        reply->deleteLater();
        if (!m_active.contains(id))
            return;
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status == 401 || status == 403) {
            fail(id, {Tmdb::ErrorCode::Authentication, status, tr("TMDB authentication failed.")});
            return;
        }
        if (status != 0 && (status < 200 || status >= 300)) {
            fail(id, {Tmdb::ErrorCode::Http, status, tr("TMDB HTTP request failed.")});
            return;
        }
        if (reply->error() != QNetworkReply::NoError || status == 0) {
            fail(id, {Tmdb::ErrorCode::Network, status, tr("TMDB network request failed.")});
            return;
        }
        const QByteArray body = reply->readAll();
        if (body.trimmed().isEmpty()) {
            fail(id, {Tmdb::ErrorCode::InvalidResponse, status, tr("Empty TMDB response.")});
            return;
        }
        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(body, &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            fail(id, {Tmdb::ErrorCode::JsonParsing, status, tr("Malformed TMDB JSON response.")});
            return;
        }
        QPointer<TmdbPrivate> guard(this);
        if (!document.isObject() || document.object().value("success") == QJsonValue(false)
            || !done(document.object())) {
            if (guard)
                fail(id, {Tmdb::ErrorCode::InvalidResponse, status, tr("Unexpected TMDB response fields.")});
        }
    }

    Tmdb *q;
    QNetworkAccessManager *m_manager;
    QString m_token;
    Tmdb::RequestId m_nextId = 0;
    QHash<Tmdb::RequestId, QNetworkReply *> m_active;
};

Tmdb::Tmdb(QObject *parent) : QObject(parent), d(new TmdbPrivate(this))
{
    qRegisterMetaType<Tmdb::RequestId>("Streamable::Tmdb::RequestId");
    qRegisterMetaType<FilmSearchResult>();
    qRegisterMetaType<SeriesSearchResult>();
    qRegisterMetaType<Tmdb::Error>();
    qRegisterMetaType<Tmdb::State>();
    qRegisterMetaType<Film>();
    qRegisterMetaType<Series>();
    qRegisterMetaType<Season>();
    qRegisterMetaType<Episode>();
    qRegisterMetaType<QVector<TrailerData>>();
    qRegisterMetaType<Tmdb::ContentType>();
}

Tmdb::~Tmdb() { delete d; }
void Tmdb::setAccessToken(const QString &accessToken) { d->m_token = accessToken.trimmed(); }
bool Tmdb::cancelRequest(RequestId requestId) { return d->cancel(requestId); }

Tmdb::RequestId Tmdb::searchFilms(const QString &query, int page)
{
    QUrlQuery parameters;
    parameters.addQueryItem(QStringLiteral("query"), QString::fromLatin1(QUrl::toPercentEncoding(query)));
    parameters.addQueryItem(QStringLiteral("page"), QString::number(page));
    parameters.addQueryItem(QStringLiteral("include_adult"), QStringLiteral("false"));
    return d->start(QStringLiteral("search/movie"), parameters, !query.trimmed().isEmpty() && page > 0 && page <= 500,
                    [this, page](RequestId id) {
        return [this, id, page](const QJsonObject &object) {
            const auto result = parseSearch<FilmSearchResult>(object, page, parseFilm);
            if (!result)
                return false;
            d->succeed(id, *result, &Tmdb::filmsFetched);
            return true;
        };
    });
}

Tmdb::RequestId Tmdb::searchSeries(const QString &query, int page)
{
    QUrlQuery parameters;
    parameters.addQueryItem(QStringLiteral("query"), QString::fromLatin1(QUrl::toPercentEncoding(query)));
    parameters.addQueryItem(QStringLiteral("page"), QString::number(page));
    parameters.addQueryItem(QStringLiteral("include_adult"), QStringLiteral("false"));
    return d->start(QStringLiteral("search/tv"), parameters, !query.trimmed().isEmpty() && page > 0 && page <= 500,
                    [this, page](RequestId id) {
        return [this, id, page](const QJsonObject &object) {
            const auto result = parseSearch<SeriesSearchResult>(object, page, [](const QJsonObject &item) {
                return parseSeries(item);
            });
            if (!result)
                return false;
            d->succeed(id, *result, &Tmdb::seriesFetched);
            return true;
        };
    });
}

Tmdb::RequestId Tmdb::fetchFilmDetails(int filmId)
{
    QUrlQuery parameters;
    parameters.addQueryItem(QStringLiteral("append_to_response"), QStringLiteral("videos"));
    return d->start(QStringLiteral("movie/%1").arg(filmId), parameters, filmId > 0, [this, filmId](RequestId id) {
        return [this, id, filmId](const QJsonObject &object) {
            const std::optional<Film> film = parseFilm(object);
            if (!film || film->m_id != filmId)
                return false;
            d->succeed(id, *film, &Tmdb::filmDetailsFetched);
            return true;
        };
    });
}

Tmdb::RequestId Tmdb::fetchSeriesDetails(int seriesId)
{
    QUrlQuery parameters;
    parameters.addQueryItem(QStringLiteral("append_to_response"), QStringLiteral("videos,recommendations"));
    return d->start(QStringLiteral("tv/%1").arg(seriesId), parameters, seriesId > 0, [this, seriesId](RequestId id) {
        return [this, id, seriesId](const QJsonObject &object) {
            const std::optional<Series> series = parseSeries(object, true);
            if (!series || series->m_id != seriesId)
                return false;
            d->succeed(id, *series, &Tmdb::seriesDetailsFetched);
            return true;
        };
    });
}

Tmdb::RequestId Tmdb::fetchSeason(int seriesId, int seasonNumber)
{
    return d->start(QStringLiteral("tv/%1/season/%2").arg(seriesId).arg(seasonNumber), {},
                    seriesId > 0 && seasonNumber >= 0, [this, seasonNumber](RequestId id) {
        return [this, id, seasonNumber](const QJsonObject &object) {
            const std::optional<Season> season = parseSeason(object, true);
            if (!season || season->m_number != seasonNumber)
                return false;
            d->succeed(id, *season, &Tmdb::seasonFetched);
            return true;
        };
    });
}

Tmdb::RequestId Tmdb::fetchEpisode(int seriesId, int seasonNumber, int episodeNumber)
{
    return d->start(QStringLiteral("tv/%1/season/%2/episode/%3").arg(seriesId).arg(seasonNumber).arg(episodeNumber), {},
                    seriesId > 0 && seasonNumber >= 0 && episodeNumber > 0, [this, seasonNumber, episodeNumber](RequestId id) {
        return [this, id, seasonNumber, episodeNumber](const QJsonObject &object) {
            const std::optional<Episode> episode = parseEpisode(object);
            if (!episode || episode->m_season != seasonNumber || episode->m_episode != episodeNumber)
                return false;
            d->succeed(id, *episode, &Tmdb::episodeFetched);
            return true;
        };
    });
}

Tmdb::RequestId Tmdb::fetchTrailers(ContentType contentType, int contentId)
{
    const QString path = contentType == ContentType::Movie ? QStringLiteral("movie/%1/videos") : QStringLiteral("tv/%1/videos");
    return d->start(path.arg(contentId), {}, contentId > 0 && (contentType == ContentType::Movie || contentType == ContentType::Series),
                    [this, contentType, contentId](RequestId id) {
        return [this, id, contentType, contentId](const QJsonObject &object) {
            const std::optional<QVector<TrailerData>> videos = parseVideos(object);
            if (!videos || integer(object.value("id")) != contentId)
                return false;
            d->m_active.remove(id);
            QPointer<Tmdb> guard(this);
            Q_EMIT requestStateChanged(id, State::Ready);
            if (guard)
                Q_EMIT guard->trailersFetched(id, contentType, contentId, *videos);
            return true;
        };
    });
}

} // namespace Streamable
