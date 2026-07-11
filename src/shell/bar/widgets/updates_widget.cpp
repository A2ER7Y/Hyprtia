#include "shell/bar/widgets/updates_widget.h"

#include "core/process/process.h"
#include "i18n/i18n.h"
#include "render/core/renderer.h"
#include "render/scene/input_area.h"
#include "render/scene/node.h"
#include "ui/builders.h"
#include "ui/controls/glyph.h"
#include "ui/controls/label.h"
#include "ui/palette.h"
#include "ui/style.h"

#include <algorithm>
#include <cmath>
#include <linux/input-event-codes.h>
#include <memory>
#include <string>
#include <utility>

UpdatesWidget::UpdatesWidget(PackageUpdateService* updates, Options options)
    : m_updates(updates), m_options(std::move(options)) {}

void UpdatesWidget::create() {
  auto area = std::make_unique<InputArea>();
  area->setAcceptedButtons(InputArea::buttonMask({BTN_LEFT, BTN_RIGHT}));
  area->setOnClick([this](const InputArea::PointerData& data) {
    if (data.button == BTN_RIGHT && !m_options.command.empty()) {
      (void)process::runAsync(m_options.command);
      return;
    }
    if (data.button == BTN_LEFT && m_updates != nullptr) {
      m_updates->refreshNow();
    }
  });
  m_area = area.get();

  area->addChild(
      ui::glyph({
          .out = &m_glyph,
          .glyph = m_options.glyph.empty() ? "download" : m_options.glyph,
          .glyphSize = Style::baseGlyphSize * m_contentScale,
          .color = widgetIconColorOr(colorSpecFromRole(ColorRole::OnSurfaceVariant)),
      })
  );
  area->addChild(
      ui::label({
          .out = &m_label,
          .text = "0",
          .fontSize = Style::fontSizeBody * m_contentScale,
          .fontWeight = labelFontWeight(),
          .fontFamily = labelFontFamily(),
          .color = widgetForegroundOr(colorSpecFromRole(ColorRole::OnSurface)),
          .maxLines = 1,
          .visible = m_options.showLabel,
      })
  );
  setRoot(std::move(area));
  syncState();
}

void UpdatesWidget::doLayout(Renderer& renderer, float containerWidth, float containerHeight) {
  Node* rootNode = root();
  if (rootNode == nullptr || m_glyph == nullptr || m_label == nullptr) {
    return;
  }
  syncState();

  const bool hasUpdates = m_haveState && m_lastState.total() > 0;
  const ColorSpec accent =
      hasUpdates ? colorSpecFromRole(ColorRole::Primary) : colorSpecFromRole(ColorRole::OnSurfaceVariant);
  m_glyph->setGlyphSize(Style::baseGlyphSize * m_contentScale);
  m_glyph->setColor(widgetIconColorOr(accent));
  m_glyph->measure(renderer);
  const bool vertical = containerHeight > containerWidth;
  m_label->setVisible(m_options.showLabel);
  m_label->setParticipatesInLayout(m_options.showLabel);
  m_label->setFontSize((vertical ? Style::fontSizeCaption : Style::fontSizeBody) * m_contentScale);
  m_label->setFontWeight(labelFontWeight());
  m_label->setColor(
      widgetForegroundOr(hasUpdates ? colorSpecFromRole(ColorRole::Primary) : colorSpecFromRole(ColorRole::OnSurface))
  );
  if (m_options.showLabel) {
    m_label->measure(renderer);
  } else {
    m_glyph->setPosition(0.0f, 0.0f);
    rootNode->setSize(m_glyph->width(), m_glyph->height());
    return;
  }

  const float spacing = Style::spaceXs * m_contentScale;
  if (vertical) {
    const float width = std::max(m_glyph->width(), m_label->width());
    const float height = m_glyph->height() + spacing + m_label->height();
    m_glyph->setPosition(std::round((width - m_glyph->width()) * 0.5f), 0.0f);
    m_label->setPosition(std::round((width - m_label->width()) * 0.5f), m_glyph->height() + spacing);
    rootNode->setSize(width, height);
  } else {
    const float width = m_glyph->width() + spacing + m_label->width();
    const float height = std::max(m_glyph->height(), m_label->height());
    m_glyph->setPosition(0.0f, std::round((height - m_glyph->height()) * 0.5f));
    m_label->setPosition(m_glyph->width() + spacing, std::round((height - m_label->height()) * 0.5f));
    rootNode->setSize(width, height);
  }
}

void UpdatesWidget::doUpdate(Renderer& /*renderer*/) { syncState(); }

void UpdatesWidget::syncState() {
  if (m_updates == nullptr || m_label == nullptr) {
    return;
  }
  const PackageUpdateSnapshot state = m_updates->snapshot();
  if (m_haveState && state == m_lastState) {
    return;
  }
  m_haveState = true;
  m_lastState = state;

  m_label->setText(state.checking ? "…" : std::to_string(state.total()));
  if (m_area != nullptr) {
    m_area->setTooltip(tooltipText(state));
  }
  if (Node* rootNode = root(); rootNode != nullptr) {
    const bool visible = !m_options.hideWhenZero || state.checking || state.total() > 0;
    rootNode->setVisible(visible);
    rootNode->setParticipatesInLayout(visible);
    rootNode->markLayoutDirty();
  }
  requestRedraw();
}

std::string UpdatesWidget::tooltipText(const PackageUpdateSnapshot& state) const {
  auto count = [](int value) { return value < 0 ? std::string{"—"} : std::to_string(value); };
  std::string tooltip = state.checking ? i18n::tr("bar.widgets.updates.checking")
                                       : i18n::tr("bar.widgets.updates.available", "count", state.total());
  tooltip += "\n"
      + i18n::tr("bar.widgets.updates.sources", "arch", count(state.arch), "aur", count(state.aur), "flatpak",
                 count(state.flatpak), "snap", count(state.snap), "appimage", count(state.appimage), "hyprtia",
                 count(state.hyprtia));
  tooltip += "\n" + i18n::tr("bar.widgets.updates.refresh-action");
  if (!m_options.command.empty()) {
    tooltip += " · " + i18n::tr("bar.widgets.updates.command-action");
  }
  return tooltip;
}
