#pragma once

#include "shell/bar/widget.h"
#include "system/icon_resolver.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

struct wl_output;
class Glyph;
class Image;
class InputArea;
class Label;
class Node;
class NotificationManager;

enum class NotificationWidgetDisplayMode : std::uint8_t {
  Bell,
  RecentApps,
};

class NotificationWidget : public Widget {
public:
  struct Options {
    bool hideWhenNoUnread = false;
    NotificationWidgetDisplayMode displayMode = NotificationWidgetDisplayMode::Bell;
    std::size_t maxAppIcons = 10;
    bool showEllipsis = true;
    float appIconSize = 18.0f;
    float iconSpacing = 4.0f;
  };

  NotificationWidget(NotificationManager* manager, wl_output* output, Options options);

  void create() override;

private:
  void doLayout(Renderer& renderer, float containerWidth, float containerHeight) override;
  void doUpdate(Renderer& renderer) override;
  void refreshIndicatorState(Renderer* renderer = nullptr);
  void syncRecentAppIcons(Renderer& renderer);

  NotificationManager* m_manager = nullptr;
  InputArea* m_area = nullptr;
  Glyph* m_glyph = nullptr;
  Node* m_dot = nullptr;
  Label* m_ellipsis = nullptr;
  std::vector<Image*> m_appIcons;
  std::vector<Glyph*> m_appFallbackGlyphs;
  std::vector<std::string> m_loadedAppKeys;
  IconResolver m_iconResolver;
  Options m_options;
  std::uint64_t m_lastChangeSerial = 0;
  std::size_t m_visibleAppCount = 0;
  bool m_appsTruncated = false;
  bool m_hasNotifications = false;
  bool m_dndEnabled = false;
};
