# Streamable App — Scoped Extension Rules

> **IMPORTANT:** This file **EXTENDS**, but **NEVER OVERRIDES**, the repository root `AGENTS.md`.
> All C++/Qt coding standards, lifecycle patterns, and architectural guidelines in the root `/AGENTS.md` are **mandatory** and remain active inside this directory.

---

### Local Scope Rules (`portable/apps/streamable/`)

1. **Global Code Style Applies:** Follow all code style rules, signal patterns, and model lifecycle guidelines from the repo-root `AGENTS.md`.
2. **Local Domain Architecture:** Before editing or creating files in this folder, consult `./ARCHITECTURE.md` in this directory. Treat it as the source of truth for local widget, model, and provider boundaries.
3. **Boundary Isolation:** Do not leak streamable-specific architecture specs outside this folder. When editing host wiring, docks, or CMake files elsewhere, refer strictly to repo-root guidelines.