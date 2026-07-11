#pragma once

#include "shell/bar/widget.h"
#include "shell/bar/widget_custom_image.h"

#include <string>

class Glyph;
class Image;
struct wl_output;

class QuickShortcutsWidget : public Widget {
public:
  QuickShortcutsWidget(
      wl_output* output, std::string barGlyphId, std::string barPosition, WidgetCustomImage customImage = {}
  );

  void create() override;

private:
  void doLayout(Renderer& renderer, float containerWidth, float containerHeight) override;

  std::string m_barGlyphId;
  std::string m_barPosition;
  WidgetCustomImage m_customImage;
  Glyph* m_glyph = nullptr;
  Image* m_image = nullptr;
};
