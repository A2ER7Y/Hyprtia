#include "shell/bar/widgets/notification_widget.h"

#include "i18n/i18n.h"
#include "notification/notification_icon_resolver.h"
#include "notification/notification_manager.h"
#include "render/core/renderer.h"
#include "render/scene/input_area.h"
#include "render/scene/node.h"
#include "ui/builders.h"
#include "ui/controls/glyph.h"
#include "ui/controls/image.h"
#include "ui/controls/label.h"
#include "ui/palette.h"
#include "ui/style.h"

#include <algorithm>
#include <cmath>
#include <linux/input-event-codes.h>
#include <memory>
#include <utility>
#include <vector>

namespace {
  constexpr float kDotBaseSize = 6.0f;
} // namespace

NotificationWidget::NotificationWidget(NotificationManager* manager, wl_output* /*output*/, Options options)
    : m_manager(manager), m_options(options) {
  m_options.maxAppIcons = std::clamp<std::size_t>(m_options.maxAppIcons, 1, 10);
  m_options.appIconSize = std::clamp(m_options.appIconSize, 8.0f, 48.0f);
  m_options.iconSpacing = std::clamp(m_options.iconSpacing, 0.0f, 24.0f);
}

void NotificationWidget::create() {
  auto area = std::make_unique<InputArea>();
  area->setAcceptedButtons(InputArea::buttonMask({BTN_LEFT, BTN_RIGHT}));
  area->setOnClick([this](const InputArea::PointerData& data) {
    if (data.button == BTN_RIGHT) {
      if (m_manager != nullptr) {
        const bool dndEnabled = m_manager->toggleDoNotDisturb();
        (void)dndEnabled;
      }
      requestRedraw();
      return;
    }
    if (data.button != BTN_LEFT) {
      return;
    }
    requestPanelToggle("control-center", "notifications");
  });
  m_area = area.get();

  area->addChild(
      ui::glyph({
          .out = &m_glyph,
          .glyph = "bell",
          .glyphSize = Style::baseGlyphSize * m_contentScale,
          .color = widgetIconColorOr(colorSpecFromRole(ColorRole::OnSurface)),
      })
  );

  if (m_options.displayMode == NotificationWidgetDisplayMode::RecentApps) {
    m_appIcons.reserve(m_options.maxAppIcons);
    m_appFallbackGlyphs.reserve(m_options.maxAppIcons);
    m_loadedAppKeys.resize(m_options.maxAppIcons);
    for (std::size_t i = 0; i < m_options.maxAppIcons; ++i) {
      Image* icon = nullptr;
      area->addChild(
          ui::image({
              .out = &icon,
              .fit = ImageFit::Contain,
              .visible = false,
              .participatesInLayout = false,
          })
      );
      m_appIcons.push_back(icon);

      Glyph* fallback = nullptr;
      area->addChild(
          ui::glyph({
              .out = &fallback,
              .glyph = "bell",
              .glyphSize = m_options.appIconSize * m_contentScale,
              .color = widgetIconColorOr(colorSpecFromRole(ColorRole::OnSurface)),
              .visible = false,
              .participatesInLayout = false,
          })
      );
      m_appFallbackGlyphs.push_back(fallback);
    }

    area->addChild(
        ui::label({
            .out = &m_ellipsis,
            .text = "…",
            .fontSize = Style::fontSizeCaption * m_contentScale,
            .fontWeight = labelFontWeight(),
            .fontFamily = labelFontFamily(),
            .color = widgetForegroundOr(colorSpecFromRole(ColorRole::OnSurface)),
            .maxLines = 1,
            .visible = false,
            .participatesInLayout = false,
        })
    );
  }

  const float dotSize = kDotBaseSize * m_contentScale;
  m_dot = area->addChild(
      ui::box({
          .fill = colorSpecFromRole(ColorRole::Primary),
          .radius = dotSize * 0.5f,
          .width = dotSize,
          .height = dotSize,
          .visible = false,
          .participatesInLayout = false,
      })
  );

  setRoot(std::move(area));
  refreshIndicatorState();
}

void NotificationWidget::doLayout(Renderer& renderer, float containerWidth, float containerHeight) {
  auto* rootNode = root();
  if (m_glyph == nullptr || rootNode == nullptr) {
    return;
  }

  refreshIndicatorState(&renderer);

  m_glyph->setGlyphSize(Style::baseGlyphSize * m_contentScale);
  m_glyph->setGlyph(m_dndEnabled ? "bell-off" : "bell");
  m_glyph->setColor(widgetIconColorOr(colorSpecFromRole(ColorRole::OnSurface)));
  m_glyph->measure(renderer);

  const bool showRecentApps =
      m_options.displayMode == NotificationWidgetDisplayMode::RecentApps && !m_dndEnabled && m_visibleAppCount > 0;
  m_glyph->setVisible(!showRecentApps);
  m_glyph->setParticipatesInLayout(!showRecentApps);

  std::vector<Node*> visibleNodes;
  if (!showRecentApps) {
    for (std::size_t i = 0; i < m_appIcons.size(); ++i) {
      m_appIcons[i]->setVisible(false);
      m_appIcons[i]->setParticipatesInLayout(false);
      m_appFallbackGlyphs[i]->setVisible(false);
      m_appFallbackGlyphs[i]->setParticipatesInLayout(false);
    }
    if (m_ellipsis != nullptr) {
      m_ellipsis->setVisible(false);
      m_ellipsis->setParticipatesInLayout(false);
    }
    visibleNodes.push_back(m_glyph);
  } else {
    const float iconSize = m_options.appIconSize * m_contentScale;
    for (std::size_t i = 0; i < m_visibleAppCount; ++i) {
      Image* image = m_appIcons[i];
      Glyph* fallback = m_appFallbackGlyphs[i];
      if (image != nullptr && image->hasImage()) {
        image->setVisible(true);
        image->setParticipatesInLayout(true);
        fallback->setVisible(false);
        fallback->setParticipatesInLayout(false);
        image->setSize(iconSize, iconSize);
        visibleNodes.push_back(image);
      } else if (fallback != nullptr) {
        if (image != nullptr) {
          image->setVisible(false);
          image->setParticipatesInLayout(false);
        }
        fallback->setVisible(true);
        fallback->setParticipatesInLayout(true);
        fallback->setGlyphSize(iconSize);
        fallback->setColor(widgetIconColorOr(colorSpecFromRole(ColorRole::OnSurfaceVariant)));
        fallback->measure(renderer);
        visibleNodes.push_back(fallback);
      }
    }
    if (m_appsTruncated && m_options.showEllipsis && m_ellipsis != nullptr) {
      m_ellipsis->setVisible(true);
      m_ellipsis->setParticipatesInLayout(true);
      m_ellipsis->setFontSize(Style::fontSizeCaption * m_contentScale);
      m_ellipsis->setFontWeight(labelFontWeight());
      m_ellipsis->setColor(widgetForegroundOr(colorSpecFromRole(ColorRole::OnSurface)));
      m_ellipsis->measure(renderer);
      visibleNodes.push_back(m_ellipsis);
    }
  }

  const bool vertical = containerHeight > containerWidth;
  const float spacing = m_options.iconSpacing * m_contentScale;
  float width = 0.0f;
  float height = 0.0f;
  if (vertical) {
    for (Node* node : visibleNodes) {
      width = std::max(width, node->width());
      height += node->height();
    }
    if (visibleNodes.size() > 1) {
      height += spacing * static_cast<float>(visibleNodes.size() - 1);
    }
    float y = 0.0f;
    for (Node* node : visibleNodes) {
      node->setPosition(std::round((width - node->width()) * 0.5f), y);
      y += node->height() + spacing;
    }
  } else {
    for (Node* node : visibleNodes) {
      width += node->width();
      height = std::max(height, node->height());
    }
    if (visibleNodes.size() > 1) {
      width += spacing * static_cast<float>(visibleNodes.size() - 1);
    }
    float x = 0.0f;
    for (Node* node : visibleNodes) {
      node->setPosition(x, std::round((height - node->height()) * 0.5f));
      x += node->width() + spacing;
    }
  }
  rootNode->setSize(width, height);

  if (m_dot != nullptr) {
    const float dotSize = kDotBaseSize * m_contentScale;
    m_dot->setPosition(width - dotSize, 0.0f);
  }
}

void NotificationWidget::doUpdate(Renderer& renderer) { refreshIndicatorState(&renderer); }

void NotificationWidget::refreshIndicatorState(Renderer* renderer) {
  const bool hasNotifications = (m_manager != nullptr) && m_manager->hasUnreadNotificationHistory();
  const bool dndEnabled = (m_manager != nullptr) && m_manager->doNotDisturb();

  if (Node* rootNode = root(); rootNode != nullptr) {
    const bool showWidget = !m_options.hideWhenNoUnread || hasNotifications;
    rootNode->setVisible(showWidget);
    rootNode->setParticipatesInLayout(showWidget);
  }

  if (renderer != nullptr && m_options.displayMode == NotificationWidgetDisplayMode::RecentApps) {
    syncRecentAppIcons(*renderer);
  }

  const bool stateChanged = hasNotifications != m_hasNotifications || dndEnabled != m_dndEnabled;
  m_hasNotifications = hasNotifications;
  m_dndEnabled = dndEnabled;
  if (m_area != nullptr) {
    std::string tooltip;
    if (m_dndEnabled) {
      tooltip = i18n::tr("bar.widgets.notifications.dnd");
    } else if (m_hasNotifications) {
      tooltip = i18n::tr("bar.widgets.notifications.unread");
    } else {
      tooltip = i18n::tr("bar.widgets.notifications.empty");
    }
    tooltip += "\n" + i18n::tr("bar.widgets.notifications.actions");
    m_area->setTooltip(std::move(tooltip));
  }
  if (m_glyph != nullptr) {
    m_glyph->setGlyph(m_dndEnabled ? "bell-off" : "bell");
    m_glyph->setColor(widgetIconColorOr(colorSpecFromRole(ColorRole::OnSurface)));
  }
  if (m_dot != nullptr) {
    const bool bellVisible =
        m_options.displayMode == NotificationWidgetDisplayMode::Bell || m_visibleAppCount == 0 || m_dndEnabled;
    m_dot->setVisible(m_hasNotifications && !m_dndEnabled && bellVisible);
  }
  if (stateChanged) {
    if (Node* rootNode = root(); rootNode != nullptr) {
      rootNode->markLayoutDirty();
    }
    requestRedraw();
  }
}

void NotificationWidget::syncRecentAppIcons(Renderer& renderer) {
  if (m_manager == nullptr) {
    return;
  }
  const std::uint64_t serial = m_manager->changeSerial();
  if (serial == m_lastChangeSerial) {
    return;
  }
  m_lastChangeSerial = serial;

  const auto apps = m_manager->recentNotificationApps(m_options.maxAppIcons + 1);
  m_appsTruncated = apps.size() > m_options.maxAppIcons;
  m_visibleAppCount = std::min(apps.size(), m_options.maxAppIcons);
  const int targetSize = std::max(1, static_cast<int>(std::lround(m_options.appIconSize * m_contentScale)));

  for (std::size_t i = 0; i < m_appIcons.size(); ++i) {
    const bool visible = i < m_visibleAppCount;
    Image* image = m_appIcons[i];
    Glyph* fallback = m_appFallbackGlyphs[i];
    if (!visible) {
      if (image != nullptr) {
        image->setVisible(false);
        image->setParticipatesInLayout(false);
      }
      if (fallback != nullptr) {
        fallback->setVisible(false);
        fallback->setParticipatesInLayout(false);
      }
      m_loadedAppKeys[i].clear();
      continue;
    }

    const RecentNotificationApp& app = apps[i];
    const std::string iconPath = resolveNotificationIconPath(app.icon, app.desktopEntry, m_iconResolver, targetSize);
    if (image != nullptr && m_loadedAppKeys[i] != app.key + "\n" + iconPath) {
      if (iconPath.empty() || !image->setSourceFile(renderer, iconPath, targetSize, true)) {
        image->clear(renderer);
      }
      m_loadedAppKeys[i] = app.key + "\n" + iconPath;
    }
    const bool hasImage = image != nullptr && image->hasImage();
    if (image != nullptr) {
      image->setRadius(static_cast<float>(targetSize) * 0.25f);
      image->setVisible(hasImage);
      image->setParticipatesInLayout(hasImage);
    }
    if (fallback != nullptr) {
      fallback->setVisible(!hasImage);
      fallback->setParticipatesInLayout(!hasImage);
    }
  }

  if (m_ellipsis != nullptr) {
    const bool visible = m_appsTruncated && m_options.showEllipsis;
    m_ellipsis->setVisible(visible);
    m_ellipsis->setParticipatesInLayout(visible);
  }
  if (Node* rootNode = root(); rootNode != nullptr) {
    rootNode->markLayoutDirty();
  }
  requestRedraw();
}
