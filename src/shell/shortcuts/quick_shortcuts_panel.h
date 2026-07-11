#pragma once

#include "shell/panel/panel.h"
#include "system/desktop_entry.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

class Button;
class ConfigService;
class GridView;
class Renderer;

class QuickShortcutsPanel : public Panel {
public:
  explicit QuickShortcutsPanel(ConfigService* config) : m_config(config) {}

  void create() override;

  [[nodiscard]] float preferredWidth() const override;
  [[nodiscard]] float preferredHeight() const override;
  [[nodiscard]] bool hasDecoration() const override { return true; }
  [[nodiscard]] LayerShellLayer layer() const override { return LayerShellLayer::Overlay; }
  [[nodiscard]] LayerShellKeyboard keyboardMode() const override { return LayerShellKeyboard::OnDemand; }
  [[nodiscard]] PanelPlacement panelPlacement() const noexcept override { return PanelPlacement::Attached; }

private:
  struct Entry {
    std::string label;
    std::string glyph;
    std::string command;
    bool terminal = false;
    std::optional<DesktopEntry> desktopEntry;
  };

  static constexpr float kButtonMinHeight = 92.0f;
  static constexpr float kButtonMinWidth = 124.0f;

  void doLayout(Renderer& renderer, float width, float height) override;
  [[nodiscard]] std::vector<Entry> effectiveEntries() const;
  [[nodiscard]] std::size_t entryCountForLayout() const;
  [[nodiscard]] std::size_t columns() const;
  [[nodiscard]] std::size_t rows() const;
  [[nodiscard]] Button* createEntryButton(const Entry& entry, std::size_t index, float scale);
  void activateEntry(std::size_t index);

  ConfigService* m_config = nullptr;
  GridView* m_grid = nullptr;
  std::vector<Entry> m_entries;
  std::vector<Button*> m_buttons;
};
