# Code style rules

Condensed from `doc/qtcreatordev/src/coding-style.qdoc`, which is the full
reference. Follow when writing or editing Qt Creator code.

Layout, naming and braces are not repeated here: they are visible in the
file being edited, so match the surrounding code. A new file has none, so the
rules it cannot read off its neighbours are spelled out below.

## Comments and documentation
- Put documentation into .cpp
- Do not describe the change you are making in the source: that belongs in the commit message.
- Otherwise treat a comment as an indication of a code smell, and comment only what is not evident from the code. Needing one usually means the code should be clearer.
- Out of the source entirely: notes aimed at the reviewer, where code was taken from, bug numbers. Exception: a workaround for a bug outside Creator does name it (`// Work around QTBUG-12345.`).
- When editing an existing comment, keep the wording close to the original.

## Private classes
- `d`/`q` pointers are named `d`/`q`, not `m_d`; type `FooPrivate *` / `Foo *`. Don't wrap `d` in a smart pointer (compile/link overhead, more symbols).
- `FooPrivate` is declared in the same namespace as `Foo`, or in the corresponding `Internal` namespace if `Foo` is exported. It may be a friend of `Foo` if needed (e.g. to emit its signals).

## Namespaces
- No using-directives in headers; don't rely on them for defining classes/functions or accessing global functions. Otherwise OK: place near top after includes (never `#include` after a using-directive).
- Exported symbols go in a plugin/lib namespace (`MyPlugin`), non-exported ones in `MyPlugin::Internal`.
- Qualify calls to free functions from the `Utils` namespace with `Utils::`, even where a using-directive makes it unnecessary.

## C++ features
- `#pragma once`, not header guards. No exceptions, RTTI, `dynamic_cast`, or virtual inheritance unless truly needed.
- ASCII-only source (use `\nnn`/`\xnn` escapes; in docs use qdoc `\unicode` or the relevant macro).
- `static` over anonymous namespaces (anonymous namespaces mandate external linkage).
- Use `auto` only to avoid repeating a type in the same statement or for iterators; skip if it hurts readability.
- Non-static data member init for trivial cases, except public exported classes.
- Use `=default`/`=delete`. Use `final` for non-inheritable classes and terminal overrides; `override` otherwise; never `virtual` on `final`. Mark all overrides in a class consistently.
- Range-based for: use `std::cref()` if read-only and constness/sharing unclear (avoid detach).
- `std::optional`: avoid throwing `value()`; check then use `*`/`->` or `value_or()`.

## QObject
- Add `Q_OBJECT` only to subclasses using the meta-object system. Prefer Qt5-style `connect()`.
- Avoid `QObject::sender()` - pass sender explicitly via lambda capture. Avoid `QSignalMapper` (use a lambda).

## Passing file names
- Creator API expects portable format (slashes, also on Windows).

## Classes to use / not to use
- Check `src/libs/utils/` before writing a helper: `result.h`, `expected.h`, `filepath.h`, `store.h` and `async.h` cover a lot. Don't reinvent them, and consider whether a new piece is generic enough to belong in Qt rather than Creator.
- `Utils::FilePath` for any QString that semantically is a file or directory; prefer it over `QDir`/`QFileInfo`.
- Prefer `Utils::Process` over `QProcess`.
- If `Utils::FilePath`/`Utils::Process` are insufficient, enhance them rather than fall back to `QString`/`QProcess`.
- Avoid platform `#ifdef`s unless needed for locally executed code; even then prefer `Utils::HostInfo`.

## Assertions
- Use `QTC_ASSERT(cond, action)` (runs `action`, typically `return`, `return {}`, `continue`, `break`), `QTC_CHECK(cond)` (reports only) or `QTC_GUARD(cond)` (reports and evaluates to the condition) from `utils/qtcassert.h`, not `Q_ASSERT`.
- Unlike `Q_ASSERT` these also report in release builds and none of them aborts.

## Plugin dependencies
- Keep hard run-time dependencies between plugins and to external libraries as few as reasonably possible.
- Callback pattern: a leaf plugin injects functionality into a central plugin via a `std::function` accessor (`std::function<void(...)> &fancyLeafCallback();` returning a function-scope static), so the central plugin need not depend on the leaf's dependencies. The central plugin checks the callback and falls back if unset.

## New files
- Start with the same header comment as other Creator source files.
- Include order is specific to generic: own header, other class in the plugin, `<otherplugin/...>`, `<QtClass>`, `<stdthing>`, `<system.h>`. Angle brackets for other plugins' headers, a blank line between the blocks, alphabetical inside one.

## Platform / portability
- Beware `?:` with differing types (may crash). Beware alignment when casting pointers to a type with stricter alignment, use a union to force correct alignment.
- Static header declarations: integral types / arrays / structs only.
- Function-scope statics are OK (not reentrant).
- `char` signedness is platform-dependent, use `signed char`/`uchar` explicitly. Avoid 64-bit enum values. Don't mix const/non-const iterators. Don't inline virtual destructors in exported classes (vtable duplication / RTTI break).

## Esthetics & design
- Prefer unscoped enums over `static const int`/defines for constants. Verbose argument names in headers.

## Additional Rules
### Rule: Variable Scope & Inline Instantiation
- **Principle:** Avoid repetitive, manual setup and unnecessary single-use local variables.
- **Guideline:** 
  1. Prefer inline instantiation over step-by-step, element-by-element boilerplate setup when passing objects directly into managing containers or data structures.
  2. Batch-configure uniform properties via collection iteration rather than keeping separate local variables for each object.
  3. Query container instances or registries by index/key for state-specific access instead of maintaining long-lived local handles.

### Rule: Hot-Path Performance & Production Realism (Overrides Pure SOLID Principles)

1. **Avoid Over-Decomposition in Hot Loops:**
   - **Guideline:** For high-frequency, performance-critical methods (e.g., `paint()`, render loops, event handlers), do NOT decompose the logic into small helper methods unless code is reused across multiple top-level methods.
   - **Reasoning:** Minimize function call overhead, stack frames, and parameter passing. Keep the entire painting sequence inline within a single monolithic control flow.

2. **Use Dynamic Flow Geometry (No Hardcoded Grid Assumptions):**
   - **Guideline:** Calculate positions dynamically relative to real rendered metrics rather than splitting bounding boxes into static/hardcoded proportions.
   - **Practice:** Use returned bounding rects (e.g., `painter->drawText(..., &bounding)`) to dynamically adjust subsequent bounds (`r2.adjust(0, bounding.bottom() - r2.top(), 0, 0)`). 
   - **Constraint:** Avoid `constexpr` layout padding or static line-height assumptions when font metrics or content dimensions vary.

3. **Pixel-Precision & Theme Resilience Over Abstract Defaults:**
   - **Guideline:** Account for real-world desktop environments and theme edge-cases explicitly.
   - **Practice:** 
     - Align stroke widths to integer pixel boundaries (e.g., `penWidth += penWidth % 2`).
     - Calculate aspect ratios dynamically from actual pixmap dimensions (`r.height() * pix.width() / pix.height()`) rather than forcing pre-scaled aspect ratios or drawing empty fallback boxes (`fillRect`).
     - Adjust opacity and palette colors specifically for selected vs. unselected contrast issues in system themes (e.g., Breeze, Adwaita).

### Rule: Inline Event Wiring Over Single-Use Method Extraction

Avoid polluting class interfaces and headers with single-use event handler methods (`onButtonClicked()`, `handleImport()`). Prefer inline lambdas and anonymous callbacks for signal wiring and local event handling.

#### Guidelines

1. **Keep Callback Logic Colocated:**
   - **Guideline:** Define single-use event listeners and signal callbacks directly at the site of registration using inline lambdas/anonymous functions.
   - **Reasoning:** Keeps setup logic visually cohesive in one place. Developers reading the initialization code do not need to jump to a distant class member to understand what happens on interaction.

2. **Eliminate Header & Scope Pollution:**
   - **Guideline:** Do not create private member functions or slots solely to satisfy rigid "one method per action" principles if the function is only invoked by a single event binding.
   - **Avoid:** Creating boilerplate handlers like `private: void onImportButtonClicked()` when `button_import` is only wired in a single constructor line.

3. **When to Extract to a Named Method (Extraction Threshold):**
   - Extract logic out of an inline callback into a dedicated named function ONLY when:
     1. The action must be triggered from **multiple event sources** (e.g., both a menu item and a toolbar button).
     2. The operation requires **direct, isolated unit testing** independent of UI signaling.
     3. The callback logic becomes excessively long or deeply nested, degrading readability of the surrounding setup block.

---

#### Comparison Example
#### Recommended Style (Colocated Inline Lambda)
```cpp
// Implementation (.cpp) - Context preserved inline, no header pollution
connect(button_import, &QAbstractButton::clicked, this, [this]() {
    if (m_currentProvider->get()->downloadOAuth2()) {
        m_currentProvider->get()->authorize();
    } else {
        slotSaveItem();
    }
});
```
##### Anti-Pattern (Over-extracted "Clean Code" style)
```cpp
// Header (.h) - Header bloat for a single-use slot
private:
    void handleImportButtonClicked();

// Implementation (.cpp) - Context split between setup and handler
void MyWidget::setupUI() {
    connect(button_import, &QAbstractButton::clicked, this, &MyWidget::handleImportButtonClicked);
}

void MyWidget::handleImportButtonClicked() {
    if (m_currentProvider->get()->downloadOAuth2()) {
        m_currentProvider->get()->authorize();
    } else {
        slotSaveItem();
    }
}
```
---


### Rule: Event-Driven State Broadcasting (Anti-Callback Pattern)

- **Principle:** Prefer state-machine signals over request-response callbacks or lambda passing across service boundaries.
- **Guideline:**
  1. **Broadcast Facts, Not Directives:** Services must emit operational state transitions (`stateChanged(State)`), not UI instructions (`showSpinner()`).
  2. **Multi-Observer Isolation:** UI components must passively subscribe to service signals. Services must never hold references to UI widgets or accept UI mutation callbacks.
  3. **Lifetime Protection:** Use Qt signal-slot connections (`QObject::connect`) instead of raw `std::function` closures for asynchronous operations to guarantee automatic disconnection if UI receivers are destroyed before task completion.
---

# Style Guide: Infrastructure Tooling & Internal Component Paradigm

When building low-level UI tools, IDE-style widgets, utility panels, embedded widgets, or reusable system components, follow a pragmatic, infrastructure-first implementation style rather than heavy domain-driven application architectures (e.g., strict MVC/MVVM or multi-layered screen abstractions).

---

## Core Guidelines

### 1. Single-File Colocation of Internal Helpers
- **Guideline:** Keep tightly coupled internal supporting elements (private delegates, popups, specialized data models, sub-views) inside the primary component's single implementation file.
- **Practice:** Define implementation-specific helper classes, structs, or sub-routines in file-scoped or private namespaces directly alongside the main component.
- **Avoid:** Creating separate header/source files for sub-components that exist solely to serve a single parent tool.

### 2. Infrastructure-Oriented, Domain-Agnostic Naming
- **Guideline:** Name components strictly by their technical UI, structural, or data-handling roles—completely decoupled from business logic or product domain terminology.
- **Examples:** Use `CommandPalette`, `CompletionList`, `FilteredTreeProjection` instead of `UserSearchPanel`, `EpisodeResultList`, `CatalogFilterModel`.

### 3. Direct Event Machinery & Async Pipeline Integration
- **Guideline:** Embed input interception, keyboard/focus management, and asynchronous worker pipelines directly into the component's event loop logic.
- **Practice:**
  - Intercept low-level window, focus, and keyboard events directly at the component boundary (e.g., focus loss, key overrides, event filters).
  - Wire async pipelines (search matchers, file watchers, background indexing) directly to component update handlers via event/signal dispatching.
  - Schedule deferred or thread-safe state mutations using event-loop deferred calls or queue dispatching rather than complex state synchronization frameworks.

### 4. Pragmatic Single-Component Cohesion (Imperative Control)
- **Guideline:** Favor localized, procedural state manipulation over multi-layered abstractions when managing internal UI state.
- **Practice:** Allow the primary component to directly manage its sub-element lifetimes, layout geometry, internal timers, and event bindings within its own initialization and setup blocks.

### 5. Defensive Guarding & Early-Return Mechanics
- **Guideline:** Fail fast and fail silently using defensive guard clauses at the entry point of every event handler, slot, or callback.
- **Practice:** Check validity, bounds, and state constraints before execution (`if (!isValid) return;`). Guard against unexpected unmounts, null references, or out-of-bounds indices immediately upon entering a handler.

---

## Prompt Summary Checklist

When instructing an LLM to generate code in this style, include these core directives:

- **Pragmatic & Compact:** Single-file helper colocation over scattered file structures.
- **Imperative & Event-Centric:** Direct event interception, focus manipulation, and deferred event-loop scheduling.
- **Infrastructure-Driven:** Generic, reusable names instead of domain-specific business entities.
- **Defensive:** Aggressive early-return guards at all function entry points.

# Style Guide: Service Lifecycle State Ownership & Reactive UI Signaling

Async services, network managers, and background controllers must own and broadcast their own operational lifecycle states (`Idle`, `Loading`, `Ready`, `Failed`, `Canceled`). UI widgets must act solely as reactive observers of service states, rather than tracking or driving backend execution flags internally.

---

## Core Guidelines

### 1. Service Owns the Lifecycle State
- **Guideline:** Place operational state enums and state-change signals directly on the service or model handling the async workload (`Tmdb`, `TaskRunner`, `NetworkClient`).
- **Practice:** Expose unified status signals (e.g., `signals: void stateChanged(State state);`) representing discrete operational phases.
- **Reasoning:** Multiple UI widgets (e.g., search bar, status bar, detail view) can observe a single service's status simultaneously without duplicating state flags across components.

### 2. UI Widgets as Passive Observer/Reactors
- **Guideline:** UI widgets (`BrowserWidget`, `DetailWidget`) must not track operational boolean flags (`m_isLoading`, `m_hasError`) or synthesize request states.
- **Practice:** Wire the UI to subscribe to backend state signals upon initialization (`connect(m_service, &Service::stateChanged, this, ...)`). Map service states directly to visual visibility or user alerts.

### 3. State Enums Over Scattered Boolean Signals
- **Guideline:** Express lifecycle transitions through a single, cohesive enum (`Loading`, `Ready`, `Failed`, `Canceled`) rather than separate boolean signals (`loadingStarted()`, `loadingFinished()`, `errorOccurred()`).
- **Practice:** Handle state transitions using exhaustive pattern matching (`switch` or `if/else`) inside a single connected callback.

---

## Code Structure Comparison
### Recommended Style (Service-Driven Lifecycle Signal)
```cpp
// Backend Service Header (Tmdb.h)
class Tmdb : public QObject {
    Q_OBJECT
public:
    enum class State { Idle, Loading, Ready, Failed, Canceled };
    Q_ENUM(State)

signals:
    void stateChanged(Tmdb::State state);
};

// UI Widget Implementation (BrowserWidget.cpp) - Passive subscriber
BrowserWidget::BrowserWidget(Tmdb *tmdb, QWidget *parent)
    : QWidget(parent), m_tmdb(tmdb) 
{
    connect(m_tmdb, &Tmdb::stateChanged, this, [this](Tmdb::State state) {
        const bool isLoading = (state == Tmdb::State::Loading);
        m_loadingIndicator->setVisible(isLoading);

        if (state == Tmdb::State::Failed) {
            m_messageBox->setText(tr("Failed to load TMDB results."));
            m_messageBox->show();
        }
    });
}

```
### Anti-Pattern (Over-extracted "Clean Code" style)
```cpp
// Anti-Pattern: Service relies on UI passing callbacks or tracking status externally,
// or UI manually toggling loading states around service calls.

void BrowserWidget::onSearchTriggered(const QString &query) {
    // WRONG: UI managing service execution state manually
    m_isLoading = true;
    m_loadingIndicator->show();

    m_tmdb->fetchSearchQuery(query, [this](bool success) {
        m_isLoading = false;
        m_loadingIndicator->hide();
        if (!success) {
            m_messageBox->show();
        }
    });
}
```
---

# Style Guide: Model Lifetimes, Swapping, and Abstraction

Distinguish between **persistent page models** and **transient contextual models**. Avoid header bloat caused by declaring concrete model pointers for every possible sub-view state.

---

## Guidelines

### 1. Persistent Models (Static Ownership)
- **Use When:** A model shares the exact same lifecycle as the parent widget (e.g., a primary search model for a search panel).
- **Practice:** Declare as a private member pointer (`m_searchModel`), instantiate once in the constructor, and let the Qt object hierarchy manage its lifetime (`new SearchModel(this)`).

### 2. Transient & Contextual Models (Dynamic Model Binding)
- **Use When:** Model data changes based on user navigation, selection, or drill-down screens (e.g., Season details -> Episode details).
- **Practice:** 
  - Prefer referencing views through abstract interfaces (`QAbstractItemModel*`, `QAbstractListModel*`) or proxy models rather than holding multiple concrete model instances simultaneously.
  - Swap models dynamically on the view (`view->setModel(newModel)`) when context shifts.
  - Manage transient model lifetimes cleanly using `std::unique_ptr<QAbstractItemModel>` or re-parenting, destroying stale models when exiting a context rather than leaving them dormant in memory.

---

## Comparison Example
### Anti-Pattern (LLM Style: Concrete Model Pollution)
```cpp
// Header (.h) - Header polluted with multiple concrete sub-models
class DetailWidget : public QWidget {
    Q_OBJECT
private:
    SeasonModel *m_seasonModel{nullptr};   // Dormant when viewing episodes
    EpisodeModel *m_episodeModel{nullptr}; // Dormant when viewing seasons
    QListView *m_listView{nullptr};
};
```
### Recommended Style
```cpp
// Header (.h) - Decoupled view holding a generic model interface
class DetailWidget : public QWidget {
    Q_OBJECT
public:
    void setContextModel(std::unique_ptr<QAbstractItemModel> newModel);

private:
    QAbstractItemView *m_itemView{nullptr};
    std::unique_ptr<QAbstractItemModel> m_currentModel; // Clean lifetime management
};

// Implementation (.cpp) - Dynamic swapping on navigation
void DetailWidget::setContextModel(std::unique_ptr<QAbstractItemModel> newModel) {
    m_currentModel = std::move(newModel);
    m_itemView->setModel(m_currentModel.get());
}
```