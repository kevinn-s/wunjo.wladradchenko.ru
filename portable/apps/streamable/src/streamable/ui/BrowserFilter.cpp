/*
    SPDX-FileCopyrightText: 2026 Streamable Contributors
    SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

/**
 * @file BrowserFilter.cpp
 * @brief Implements the Streamable search and media-type input widget.
 */

#include "BrowserFilter.h"

#include <QAction>
#include <QButtonGroup>
#include <QEvent>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QIcon>
#include <QLineEdit>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>

namespace Streamable {

static constexpr int kSearchFontPixelSize = 20;
static constexpr int kTypeFontPixelSize = 16;

static QFont streamableFont(const QFont &baseFont, int pixelSize, QFont::Weight weight)
{
    QFont font = baseFont;
    font.setFamily(QStringLiteral("Plus Jakarta Sans"));
    font.setPixelSize(pixelSize);
    font.setWeight(weight);
    return font;
}

BrowserFilter::BrowserFilter(QWidget *parent)
    : QWidget(parent)
    , m_searchEdit(new QLineEdit(this))
    , m_typeButtonGroup(new QButtonGroup(this))
    , m_filmButton(new QToolButton(this))
    , m_seriesButton(new QToolButton(this))
{
    const QFontMetrics fontMetrics(font());
    const int spacing = qMax(8, fontMetrics.height() / 2);
    m_searchEdit->setPlaceholderText(tr("Search..."));
    m_searchEdit->setClearButtonEnabled(true);
    m_searchEdit->setFont(streamableFont(font(), kSearchFontPixelSize, QFont::Medium));
    m_searchEdit->setMinimumHeight(QFontMetrics(m_searchEdit->font()).height() * 3);

    const QIcon searchIcon = QIcon::fromTheme(QStringLiteral("edit-find"),
                                               style()->standardIcon(QStyle::SP_FileDialogContentsView));
    m_searchAction = m_searchEdit->addAction(searchIcon, QLineEdit::LeadingPosition);
    m_searchAction->setToolTip(tr("Search"));
    m_searchAction->setText(tr("Search"));
    connect(m_searchEdit, &QLineEdit::returnPressed, this, &BrowserFilter::SubmitSearch);
    connect(m_searchAction, &QAction::triggered, this, &BrowserFilter::SubmitSearch);

    m_typeButtonGroup->setExclusive(true);
    m_typeButtonGroup->addButton(m_filmButton, int(SearchType::Film));
    m_typeButtonGroup->addButton(m_seriesButton, int(SearchType::Series));
    for (QToolButton *button : {m_filmButton, m_seriesButton}) {
        button->setCheckable(true);
        button->setAutoRaise(true);
        button->setToolButtonStyle(Qt::ToolButtonTextOnly);
        button->setFont(streamableFont(font(), kTypeFontPixelSize, QFont::Bold));
        button->setMinimumHeight(QFontMetrics(button->font()).height() * 3);
        button->setMinimumWidth(QFontMetrics(button->font()).horizontalAdvance(tr("Series"))
                                + QFontMetrics(button->font()).height() * 3);
        button->setFocusPolicy(Qt::StrongFocus);
    }
    m_filmButton->setText(tr("Film"));
    m_seriesButton->setText(tr("Series"));
    m_filmButton->setAccessibleName(tr("Film"));
    m_seriesButton->setAccessibleName(tr("Series"));
    m_filmButton->setChecked(true);
    connect(m_typeButtonGroup, &QButtonGroup::idClicked,
        this, [this](int) {
            SubmitSearch();
        });
    updateTypeButtonStyle();

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(spacing, spacing / 2, spacing, spacing / 2);
    layout->setSpacing(spacing / 2);

    layout->addWidget(m_searchEdit);

    QHBoxLayout *typeLayout = new QHBoxLayout;
    typeLayout->setContentsMargins(0, 0, 0, 0);
    typeLayout->setSpacing(0);
    typeLayout->addStretch(1);
    typeLayout->addWidget(m_filmButton);
    typeLayout->addWidget(m_seriesButton);
    typeLayout->addStretch(1);
    layout->addLayout(typeLayout);
}

void BrowserFilter::SubmitSearch()
{
    const QString query = m_searchEdit->text().trimmed();
    if (query.isEmpty())
        return;
    Q_EMIT SearchRequested(query, static_cast<SearchType>(m_typeButtonGroup->checkedId()));
}

void BrowserFilter::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::PaletteChange || event->type() == QEvent::StyleChange)
        updateTypeButtonStyle();
}

void BrowserFilter::updateTypeButtonStyle()
{
    const QColor highlight = palette().color(QPalette::Highlight);
    const QColor highlightedText = palette().color(QPalette::HighlightedText);
    const QColor hover = palette().color(QPalette::AlternateBase);
    const QColor text = palette().color(QPalette::ButtonText);
    const int radius = QFontMetrics(m_filmButton->font()).height() * 3 / 2;
    const QString styleSheet = QStringLiteral(
        "QToolButton { border: 1px solid transparent; border-radius: %1px; padding: %2px; "
        "background: transparent; color: %3; }"
        "QToolButton:hover { background: %4; }"
        "QToolButton:checked { background: %10; color: %6; }"
        "QToolButton:focus { border-color: %5; }")
                                   .arg(radius)
                                   .arg(QFontMetrics(m_filmButton->font()).height() / 2)
                                   .arg(text.name(), hover.name(), highlight.name(), highlightedText.name());
    m_filmButton->setStyleSheet(styleSheet);
    m_seriesButton->setStyleSheet(styleSheet);
}

} // namespace Streamable
