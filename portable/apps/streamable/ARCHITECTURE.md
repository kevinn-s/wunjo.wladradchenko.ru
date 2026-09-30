# STREAMABLE: Architectural Specification & Implementation Plan
**Target Application:** Native Qt/C++ Extension for Kdenlive / Wunjo Make (`QDockWidget`)  
**Design Pattern:** Asynchronous Decoupled Execution & Headless Media Extraction Engine  
**Version:** 1.0.0-draft

---

## 1. App Overview & System Goals
`Streamable` is a native Qt/C++ dockable workspace panel (`QDockWidget`) that integrates TMDb metadata discovery with Cloudstream-like headless media extraction. 

### Core Capabilities:
1. **Metadata & Discovery:** Browse movies and TV series directly via TMDb API endpoints.
2. **Headless Extraction:** Direct link extraction bypasses browsers, JS popups, and ad overlays using C++ network requests (`QNetworkAccessManager`) and C++ string/regex parsing.
3. **Native Kdenlive/Wunjo Integration:** Directly feeds resolved `.m3u8` (HLS) or `.mp4` URLs into Kdenlive.

---

## 2. Current Directory Structure
### Models, Views, and Widgets
```text
src/streamable/
├── CMakeLists.txt
├── models/
│   ├─- misc/
│   ├── ├── TrailerData.h
│   ├── Film.h
│   ├── Series.h
│   ├── Season.h 
│   ├── Episode.h 
│   ├── FilmModel.h / .cpp       # QAbstractListModel wrapper for Movies
│   ├── SeriesModel.h / .cpp     # QAbstractListModel wrapper for TV Shows
│   ├── SeasonModel.h / .cpp     # QAbstractListModel wrapper for Seasons
│   └── EpisodeModel.h / .cpp    # QAbstractListModel wrapper for Episodes
├── ui/
│   ├── Streamable.h / .cpp  # QDockWidget Container
│   ├── BrowserWidget.h / .cpp   # Main QWidget Root Container
│   ├── BrowserFilter.h / .cpp   # Search & Genre Filter Toolbar
│   ├── DiscoverView.h / .cpp    # Grid View for Movies/Series
│   └── ResultView.h / .cpp      # Detailed Overview (Seasons, Episodes, Action Buttons)    
```

---

## 3. Production-Grade Implementation References

For non-trivial architectural and implementation decisions, use **DeepWiki MCP** to study comparable implementations in mature, production-grade open-source projects.

Do not rely only on general knowledge or invent a design when an established project provides a relevant implementation to study.

### When to Use DeepWiki

Use DeepWiki MCP when deciding or reviewing:

- class and component responsibilities;
- ownership and lifetime relationships;
- function and method declarations;
- parameter and return-value design;
- public and internal API boundaries;
- state-management patterns;
- model/view/controller responsibilities;
- signals, callbacks, events, and asynchronous flows;
- dependency injection and service ownership;
- navigation and page lifecycle;
- error and loading-state handling;
- implementation patterns and architectural conventions;
- the mental model and design rationale behind a comparable subsystem.

For Qt/C++ architecture, prefer relevant mature projects such as **Qt Creator, KDE Discover, Kdenlive, and other established Qt applications**.

### Reference Workflow

Before introducing a significant new abstraction, responsibility boundary, or API:

1. Identify a mature open-source project that solves an analogous problem.
2. Use **DeepWiki MCP** to inspect its architecture and relevant implementation.
3. Ask targeted questions about the specific classes, functions, ownership relationships, and execution flow involved.
4. Determine the **mental model and design rationale** behind the implementation rather than copying code mechanically.
5. Compare the reference with this project's existing architecture and constraints.
6. Adapt the pattern only when its reasoning applies to this codebase.

DeepWiki queries should be specific. Prefer questions such as:

- "How does `qt-creator/qt-creator` separate search UI state from search result model state?"
- "Which class owns the model, and why?"
- "What is the responsibility of this function and why are these parameters passed to it?"
- "How does `KDE/discover` implement incremental loading in its `QAbstractListModel`?"
- "How are object ownership and lifetime handled between these classes?"
- "What architectural reason explains this class boundary?"
- "What established pattern could be adapted to this project's equivalent component?"

### Evidence and Authority

Treat DeepWiki as a **reference for understanding existing implementations**, not as the source of truth for this project's code.

When using an external project as architectural evidence:

- identify the repository;
- identify the relevant class, function, or subsystem;
- explain the pattern being referenced;
- explain the reasoning or mental model behind it;
- explain why that reasoning applies to the current problem.

When DeepWiki's explanation and the referenced project's source code disagree, prefer the **actual source code**.

### Preferred External Production References

#### Preferred Qt/C++ Production References

For Qt/C++ implementation and architectural decisions, use **mature, production-grade Qt applications as the primary external references** through DeepWiki MCP. Prefer studying how these projects solve an analogous problem before inventing a new class structure, ownership model, function interface, or state-management pattern.

The preferred reference projects are:

- **Qt Creator — `qt-creator/qt-creator`**  
  Use as a primary reference for Qt/C++ application architecture, widget and model responsibilities, plugin-oriented design, services, state ownership, signals and callbacks, search/navigation systems, and class/function boundaries. When a problem has an equivalent subsystem in Qt Creator, inspect the relevant classes and execution flow through DeepWiki before designing the local equivalent. [Qt Creator GitHub repository](https://github.com/qt-creator/qt-creator?utm_source=chatgpt.com)

- **KDE Discover — `KDE/discover`**  
  Use as a primary reference for KDE/Qt model-view architecture, `QAbstractItemModel` / `QAbstractListModel` usage, asynchronous and incremental data loading, UI-to-model separation, application state, backend abstraction, and ownership relationships between views, models, and services. This is particularly relevant when designing searchable or dynamically populated browser-style interfaces. [KDE Discover GitHub repository](https://github.com/KDE/discover?utm_source=chatgpt.com)

- **Kdenlive — `KDE/kdenlive`**  
  Use as a primary reference when the problem concerns integration with the existing Kdenlive application, dock or panel architecture, long-lived application components, media-related models, navigation between editor UI states, asynchronous operations, or conventions that should remain consistent with the host application. Kdenlive is itself a substantial C++ application built with Qt and KDE Frameworks. [Kdenlive GitHub repository](https://github.com/KDE/kdenlive?utm_source=chatgpt.com)

Other mature Qt/C++ projects may be used when they provide a closer analogue to the problem being solved. **Choose references based on architectural similarity, not merely because they use Qt.**

When consulting these projects through **DeepWiki MCP**, investigate more than the final code shape. Determine:

- which class owns the responsibility and why;
- how object lifetime and ownership are managed;
- why a function is declared on a particular class;
- why its parameters and return type have that form;
- which state belongs to the UI, model, controller, or service;
- how objects communicate through signals, callbacks, interfaces, or direct dependencies;
- how loading, failure, cancellation, and other state transitions are represented;
- and the underlying **mental model and design rationale** that led to those boundaries.

Use the reference implementation as **architectural evidence rather than a template to copy mechanically**. Adapt its principles to this project's constraints and existing architecture. If DeepWiki's explanation conflicts with the referenced repository's current source code, treat the **source code as authoritative**.

### Architectural Reference Implementation
Study the system decomposition, mental model data flow, and feature architecture, overall system model rather than copying code.
- **CloudStream Reference Architecture — `recloudstream/cloudstream`**
  Use as the primary architectural reference implementation for designing media discovery workflows, content detail navigation, provider/source abstraction layers, and playback pipeline mechanics (e.g., headless HTTP scraping, JS unpacking, M3U8/HLS stream extraction, and player hand-off). Analyze the repository—via DeepWiki MCP or direct inspection—to understand its decoupling models, component responsibility boundaries, asynchronous state transitions, and data flow across subsystems (e.g., mapping provider payloads `apiName` and `data` from metadata objects like `ResultEpisode` into low-level `ExtractorLink` stream emitters), rather than copying implementation details directly.

  ---