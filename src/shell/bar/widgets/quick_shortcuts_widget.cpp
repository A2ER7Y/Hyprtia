#include "shell/bar/widgets/quick_shortcuts_widget.h"

#include "render/scene/input_area.h"
#include "render/scene/node.h"
#include "ui/builders.h"
#include "ui/palette.h"
#include "ui/style.h"

#include <linux/input-event-codes.h>
#include <memory>
#include <utility>

QuickShortcutsWidget::QuickShortcutsWidget(
    wl_output* /*output*/, std::string barGlyphId, std::string barPosition, WidgetCustomImage customImage
)
    : m_barGlyphId(std::move(barGlyphId)), m_barPosition(std::move(barPosition)),
      m_customImage(std::move(customImage)) {}

void QuickShortcutsWidget::create() {
  auto area = std::make_unique<InputArea>();
  InputArea* areaPtr = area.get();
  area->setAcceptedButtons(InputArea::buttonMask(BTN_LEFT));
  area->setOnClick([this, areaPtr](const InputArea::PointerData& data) {
    if (data.button != BTN_LEFT) {
      return;
    }

    float x = 0.0f;
    float y = 0.0f;
    Node::absolutePosition(areaPtr, x, y);
    float anchorX = x + areaPtr->width() * 0.5f;
    float anchorY = y + areaPtr->height() * 0.5f;
    if (m_barPosition == "top") {
      anchorY += areaPtr->height() * 0.5f + Style::spaceXs * m_contentScale;
    } else if (m_barPosition == "bottom") {
      anchorY -= areaPtr->height() * 0.5f + Style::spaceXs * m_contentScale;
    } else if (m_barPosition == "left") {
      anchorX += areaPtr->width() * 0.5f + Style::spaceXs * m_contentScale;
    } else if (m_barPosition == "right") {
      anchorX -= areaPtr->width() * 0.5f + Style::spaceXs * m_contentScale;
    }
    requestPanelToggle("quick-shortcuts", configName(), anchorX, anchorY);
  });

  if (m_customImage.enabled()) {
    area->addChild(ui::image({.out = &m_image, .fit = ImageFit::Contain}));
  } else {
    area->addChild(
        ui::glyph({
            .out = &m_glyph,
            .glyph = m_barGlyphId.empty() ? "bookmark" : m_barGlyphId,
            .glyphSize = Style::baseGlyphSize * m_contentScale,
            .color = widgetIconColorOr(colorSpecFromRole(ColorRole::OnSurface)),
        })
    );
  }

  setRoot(std::move(area));
}

void QuickShortcutsWidget::doLayout(Renderer& renderer, float /*containerWidth*/, float /*containerHeight*/) {
  if (m_image != nullptr) {
    widget_custom_image::sync(
        *m_image, renderer, m_customImage, m_contentScale, widgetIconColorOr(colorSpecFromRole(ColorRole::OnSurface))
    );
    if (root() != nullptr) {
      root()->setSize(m_image->width(), m_image->height());
    }
    return;
  }
  if (m_glyph == nullptr) {
    return;
  }
  m_glyph->setGlyphSize(Style::baseGlyphSize * m_contentScale);
  m_glyph->setColor(widgetIconColorOr(colorSpecFromRole(ColorRole::OnSurface)));
  m_glyph->measure(renderer);
  if (root() != nullptr) {
    root()->setSize(m_glyph->width(), m_glyph->height());
  }
}
