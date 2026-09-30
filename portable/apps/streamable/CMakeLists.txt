# SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

set(wunjo_SRCS
    ${wunjo_SRCS}
    ../apps/streamable/src/streamable/Streamable.cpp
    ../apps/streamable/src/streamable/Tmdb.cpp
    ../apps/streamable/src/streamable/models/EpisodeModel.cpp
    ../apps/streamable/src/streamable/models/FilmModel.cpp
    ../apps/streamable/src/streamable/models/SeriesModel.cpp
    ../apps/streamable/src/streamable/ui/BrowserFilter.cpp
    ../apps/streamable/src/streamable/ui/DiscoverView.cpp
    ../apps/streamable/src/streamable/ui/delegates/DiscoverDelegate.cpp
    ../apps/streamable/src/streamable/Streamable.h
    ../apps/streamable/src/streamable/Tmdb.h
    ../apps/streamable/src/streamable/models/Episode.h
    ../apps/streamable/src/streamable/models/EpisodeModel.h
    ../apps/streamable/src/streamable/models/Film.h
    ../apps/streamable/src/streamable/models/FilmModel.h
    ../apps/streamable/src/streamable/models/Season.h
    ../apps/streamable/src/streamable/models/Series.h
    ../apps/streamable/src/streamable/models/SeriesModel.h
    ../apps/streamable/src/streamable/models/misc/TrailerData.h
    ../apps/streamable/src/streamable/ui/BrowserFilter.h
    ../apps/streamable/src/streamable/ui/DiscoverView.h
    ../apps/streamable/src/streamable/ui/delegates/DiscoverDelegate.h
    PARENT_SCOPE)
