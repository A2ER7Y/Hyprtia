#include "shell/bar/widgets/power_profile_widget.h"

#include "dbus/power/power_profiles_service.h"
#include "render/scene/input_area.h"
#include "render/scene/node.h"
#include "ui/builders.h"
#include "ui/controls/glyph.h"
#include "ui/controls/label.h"
#include "ui/palette.h"
#include "ui/style.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <wayland-client-protocol.h>

PowerProfileWidget::PowerProfileWidget(PowerProfilesService* powerProfiles, bool showState)
    : m_powerProfiles(powerProfiles), m_showState(showState) {}

void PowerProfileWidget::create() {
  auto area = std::make_unique<InputArea>();
  area->setAcceptedButtons(InputArea::buttonMask({BTN_LEFT, BTN_RIGHT}));
  // Left moves forward (toward performance), right moves backward (toward power-saver).
  area->setOnClick([this](const InputArea::PointerData& data) { cycleProfile(data.button == BTN_RIGHT ? -1 : 1); });
  area->setOnAxis([this](const InputArea::PointerData& data) {
    if (data.axis != WL_POINTER_AXIS_VERTICAL_SCROLL) {
      return;
    }
    // Scroll up moves forward; Wayland reports up as a negative delta.
    cycleProfile(data.scrollDelta(1.0f) > 0 ? -1 : 1);
  });
  m_area = area.get();

  area->addChild(
      ui::glyph({
          .out = &m_glyph,
          .glyph = "balanced",
          .glyphSize = Style::baseGlyphSize * m_contentScale,
          .color = widgetIconColorOr(colorSpecFromRole(ColorRole::OnSurface)),
      })
  );
  area->addChild(
      ui::label({
          .out = &m_stateLabel,
          .text = "OFF",
          .fontSize = Style::fontSizeCaption * m_contentScale,
          .fontWeight = FontWeight::Bold,
          .color = widgetForegroundOr(colorSpecFromRole(ColorRole::OnSurfaceVariant)),
          .maxLines = 1,
          .visible = m_showState,
      })
  );

  setRoot(std::move(area));
}

void PowerProfileWidget::doLayout(Renderer& renderer, float containerWidth, float containerHeight) {
  auto* rootNode = root();
  if (m_glyph == nullptr || m_stateLabel == nullptr || rootNode == nullptr) {
    return;
  }
  syncState(renderer);

  m_glyph->setGlyphSize(Style::baseGlyphSize * m_contentScale);
  const ColorSpec activeColor = colorSpecFromRole(ColorRole::Primary);
  const ColorSpec inactiveColor = widgetIconColorOr(colorSpecFromRole(ColorRole::OnSurfaceVariant));
  m_glyph->setColor(m_available ? (m_performanceActive ? activeColor : inactiveColor)
                                : colorSpecFromRole(ColorRole::OnSurfaceVariant));
  m_glyph->measure(renderer);
  m_stateLabel->setVisible(m_showState);
  m_stateLabel->setParticipatesInLayout(m_showState);

  if (!m_showState) {
    m_glyph->setPosition(0.0f, 0.0f);
    rootNode->setSize(m_glyph->width(), m_glyph->height());
    return;
  }

  m_stateLabel->setText(m_performanceActive ? "ON" : "OFF");
  m_stateLabel->setFontSize(Style::fontSizeCaption * m_contentScale);
  m_stateLabel->setColor(
      m_performanceActive ? widgetForegroundOr(activeColor)
                          : widgetForegroundOr(colorSpecFromRole(ColorRole::OnSurfaceVariant))
  );
  m_stateLabel->measure(renderer);

  const bool vertical = containerHeight > containerWidth;
  const float spacing = Style::spaceXs * m_contentScale;
  if (vertical) {
    const float width = std::max(m_glyph->width(), m_stateLabel->width());
    const float height = m_glyph->height() + spacing + m_stateLabel->height();
    m_glyph->setPosition(std::round((width - m_glyph->width()) * 0.5f), 0.0f);
    m_stateLabel->setPosition(std::round((width - m_stateLabel->width()) * 0.5f), m_glyph->height() + spacing);
    rootNode->setSize(width, height);
  } else {
    const float width = m_glyph->width() + spacing + m_stateLabel->width();
    const float height = std::max(m_glyph->height(), m_stateLabel->height());
    m_glyph->setPosition(0.0f, std::round((height - m_glyph->height()) * 0.5f));
    m_stateLabel->setPosition(
        m_glyph->width() + spacing, std::round((height - m_stateLabel->height()) * 0.5f)
    );
    rootNode->setSize(width, height);
  }
}

void PowerProfileWidget::doUpdate(Renderer& renderer) { syncState(renderer); }

void PowerProfileWidget::syncState(Renderer& renderer) {
  if (m_glyph == nullptr || m_area == nullptr) {
    return;
  }

  const std::string profile = m_powerProfiles != nullptr ? m_powerProfiles->activeProfile() : std::string{};
  const bool available = m_powerProfiles != nullptr && (!profile.empty() || !m_powerProfiles->profiles().empty());
  const bool performanceActive = profile == "performance";

  if (profile == m_lastProfile && available == m_available && performanceActive == m_performanceActive) {
    return;
  }

  m_available = available;
  m_lastProfile = profile;
  m_performanceActive = performanceActive;

  m_glyph->setGlyph(profileGlyphName(profile));
  m_glyph->setColor(
      m_available ? (m_performanceActive ? colorSpecFromRole(ColorRole::Primary)
                                        : widgetIconColorOr(colorSpecFromRole(ColorRole::OnSurfaceVariant)))
                  : colorSpecFromRole(ColorRole::OnSurfaceVariant)
  );
  m_glyph->measure(renderer);
  if (m_stateLabel != nullptr) {
    m_stateLabel->setText(m_performanceActive ? "ON" : "OFF");
  }
  m_area->setTooltip(m_performanceActive ? "Performance mode: ON" : "Performance mode: OFF");
  m_area->setEnabled(m_available);
  if (auto* rootNode = root(); rootNode != nullptr) {
    rootNode->setOpacity(m_available ? 1.0f : 0.55f);
    rootNode->markLayoutDirty();
  }
  requestRedraw();
}

void PowerProfileWidget::cycleProfile(int direction) {
  if (m_powerProfiles == nullptr) {
    return;
  }
  (void)m_powerProfiles->cycleActiveProfile(direction);
}
