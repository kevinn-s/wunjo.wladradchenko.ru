/*
    SPDX-FileCopyrightText: 2026 Streamable Contributors
    SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

/**
 * @file Tmdb.h
 * @brief Declares the asynchronous TMDB API service used by Streamable data
 *        models.
 *
 * Tmdb defines the network/service boundary between Streamable domain models
 * and The Movie Database (TMDB) REST API.
 *
 * The service exposes asynchronous operations for movie and TV-series search,
 * detailed media lookup, seasons, episodes, and trailers. Responses are
 * represented using Streamable domain types rather than presentation widgets or
 * raw JSON payloads.
 *
 * Tmdb contains no UI or navigation logic. FilmModel, SeriesModel, SeasonModel,
 * and EpisodeModel use the service as a non-owning dependency while retaining
 * ownership of their own query, pagination, and model state.
 *
 * Key responsibilities are as follows:
 * 1. API Contract: Defines strongly typed asynchronous TMDB operations.
 * 2. Search Requests: Supports paginated movie and TV-series search.
 * 3. Detail Requests: Supports film, series, season, episode, and trailer
 *    lookups.
 * 4. Domain Results: Reports data using Film, Series, Season, Episode, and
 *    TrailerData.
 * 5. Request State: Exposes sufficient request identity/state for callers to
 *    handle completion, failure, cancellation, and stale responses safely.
 *
 * @see Film
 * @see Series
 * @see Season
 * @see Episode
 * @see TrailerData
 */

#include "models/Episode.h"
#include "models/Film.h"
#include "models/Season.h"
#include "models/Series.h"
#include "models/misc/TrailerData.h"

#include <QMetaType>
#include <QObject>
#include <QString>
#include <QVector>

namespace Streamable {

class TmdbPrivate;

struct FilmSearchResult
{
    QVector<Film> items;
    int page = 0;
    int totalPages = 0;
    int totalResults = 0;
};

struct SeriesSearchResult
{
    QVector<Series> items;
    int page = 0;
    int totalPages = 0;
    int totalResults = 0;
};

class Tmdb final : public QObject
{
    Q_OBJECT

public:
    using RequestId = quint64;
    static constexpr RequestId InvalidRequestId = 0;

    enum class State {
        Loading,
        Ready,
        Failed,
        Canceled
    };
    Q_ENUM(State)

    enum class ErrorCode {
        Network,
        Http,
        Authentication,
        InvalidResponse,
        JsonParsing,
        InvalidRequest
    };
    Q_ENUM(ErrorCode)

    enum class ContentType {
        Movie,
        Series
    };
    Q_ENUM(ContentType)

    struct Error
    {
        ErrorCode code = ErrorCode::InvalidResponse;
        int httpStatus = 0;
        QString message;
    };

    /**
     * @brief Creates TMDB service and initializes its private networking state.
     * @param parent QObject lifetime owner. Models should receive this service
     *        as a non-owning pointer instead.
     */
    explicit Tmdb(QObject *parent = nullptr);
    ~Tmdb() override;

    Tmdb(const Tmdb &) = delete;
    Tmdb &operator=(const Tmdb &) = delete;

    /**
     * @brief Sets or replaces the bearer token used by subsequent requests.
     * @param accessToken TMDB API access token. Empty token causes protected
     *        requests to fail with Authentication.
     */
    void setAccessToken(const QString &accessToken);

    /**
     * @brief Starts one paginated movie search request.
     * @param query Search text submitted to TMDB.
     * @param page One-based TMDB result page.
     * @return Unique request identity, stable for this service lifetime.
     *         Invalid input is reported asynchronously as InvalidRequest.
     */
    RequestId searchFilms(const QString &query, int page);

    /**
     * @brief Starts one paginated TV-series search request.
     * @param query Search text submitted to TMDB.
     * @param page One-based TMDB result page.
     * @return Unique request identity, stable for this service lifetime.
     *         Invalid input is reported asynchronously as InvalidRequest.
     */
    RequestId searchSeries(const QString &query, int page);

    /** @brief Fetches full details for one movie by TMDB ID.
     *  @param filmId TMDB movie identifier.
     *  @return Unique request identity.
     */
    RequestId fetchFilmDetails(int filmId);

    /** @brief Fetches full details for one TV series by TMDB ID.
     *  @param seriesId TMDB TV-series identifier.
     *  @return Unique request identity.
     */
    RequestId fetchSeriesDetails(int seriesId);

    /** @brief Fetches one season and its episode metadata.
     *  @param seriesId Parent TMDB TV-series identifier.
     *  @param seasonNumber TMDB season number; special seasons may use number
     *         zero.
     *  @return Unique request identity.
     */
    RequestId fetchSeason(int seriesId, int seasonNumber);

    /** @brief Fetches one episode by series, season, and episode identifiers.
     *  @param seriesId Parent TMDB TV-series identifier.
     *  @param seasonNumber Parent season number.
     *  @param episodeNumber Episode number within the season.
     *  @return Unique request identity.
     */
    RequestId fetchEpisode(int seriesId, int seasonNumber, int episodeNumber);

    /** @brief Fetches video/trailer references for a movie or TV series.
     *  @param contentType Identifies TMDB movie or TV-series endpoint.
     *  @param contentId TMDB identifier for the selected content.
     *  @return Unique request identity.
     */
    RequestId fetchTrailers(ContentType contentType, int contentId);

    /**
     * @brief Attempts to cancel an outstanding request.
     * @param requestId Identity returned by a request method.
     * @return True when request was outstanding and cancellation was requested;
     *         false when request was unknown or already terminal.
     *
     * Successful cancellation emits requestStateChanged with Canceled and
     * suppresses later result/failure events for that request. A reply already
     * completed may have emitted its terminal event before cancellation.
     */
    bool cancelRequest(RequestId requestId);

Q_SIGNALS:
    /**
     * @brief Reports per-request lifecycle transitions.
     * @param requestId Identity returned by the initiating request method.
     * @param state Loading is emitted after acceptance; Ready, Failed, or
     *        Canceled is terminal. No service-wide busy state is implied.
     *
     * Ready is followed by exactly one typed result signal; Failed is followed
     * by requestFailed; Canceled suppresses result/failure signals. Every event
     * carries request identity for stale-response filtering.
     */
    void requestStateChanged(Streamable::Tmdb::RequestId requestId,
                             Streamable::Tmdb::State state);

    /**
     * @brief Reports successful movie search with pagination metadata.
     * @param requestId Identity of the request.
     * @param result Owned value copy; caller/model owns its received copy.
     */
    void filmsFetched(Streamable::Tmdb::RequestId requestId,
                      Streamable::FilmSearchResult result);

    /** @brief Reports successful TV-series search with pagination metadata. */
    void seriesFetched(Streamable::Tmdb::RequestId requestId,
                       Streamable::SeriesSearchResult result);

    /** @brief Reports successful movie detail lookup. */
    void filmDetailsFetched(Streamable::Tmdb::RequestId requestId,
                            Streamable::Film film);

    /** @brief Reports successful TV-series detail lookup. */
    void seriesDetailsFetched(Streamable::Tmdb::RequestId requestId,
                              Streamable::Series series);

    /** @brief Reports successful season lookup, including episodes if supplied. */
    void seasonFetched(Streamable::Tmdb::RequestId requestId,
                       Streamable::Season season);

    /** @brief Reports successful episode lookup. */
    void episodeFetched(Streamable::Tmdb::RequestId requestId,
                       Streamable::Episode episode);

    /** @brief Reports trailer/video references for requested content. */
    void trailersFetched(Streamable::Tmdb::RequestId requestId,
                         Streamable::Tmdb::ContentType contentType,
                         int contentId,
                         QVector<Streamable::TrailerData> trailers);

    /**
     * @brief Reports a typed failure for one request.
     * @param requestId Identity of the failed request.
     * @param error Category, optional HTTP status, and diagnostic message.
     */
    void requestFailed(Streamable::Tmdb::RequestId requestId,
                       Streamable::Tmdb::Error error);

private:
    TmdbPrivate *d = nullptr; // Owned by Tmdb; defined and destroyed in Tmdb.cpp.
};

} // namespace Streamable

Q_DECLARE_METATYPE(Streamable::FilmSearchResult)
Q_DECLARE_METATYPE(Streamable::SeriesSearchResult)
Q_DECLARE_METATYPE(Streamable::Tmdb::State)
Q_DECLARE_METATYPE(Streamable::Tmdb::ErrorCode)
Q_DECLARE_METATYPE(Streamable::Tmdb::ContentType)
Q_DECLARE_METATYPE(Streamable::Tmdb::Error)
