#include "shell/shortcuts/quick_shortcuts_panel.h"

#include "config/config_service.h"
#include "core/log.h"
#include "core/process/process.h"
#include "i18n/i18n.h"
#include "shell/dock/pinned_apps.h"
#include "shell/panel/panel_manager.h"
#include "shell/shortcuts/quick_shortcut_entry.h"
#include "system/desktop_entry_launch.h"
#include "system/terminal_launch.h"
#include "ui/builders.h"
#include "ui/controls/button.h"
#include "ui/controls/flex.h"
#include "ui/controls/grid_view.h"
#include "ui/palette.h"
#include "ui/style.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>

namespace {

  constexpr Logger kLog("shortcuts");

} // namespace

const WidgetConfig* QuickShortcutsPanel::widgetConfig() const {
  if (m_config == nullptr) {
    return nullptr;
  }
  const auto& widgets = m_config->config().widgets;
  if (!pendingOpenContext().empty()) {
    const auto it = widgets.find(std::string(pendingOpenContext()));
    if (it != widgets.end() && it->second.type == "shortcuts") {
      return &it->second;
    }
  }
  const auto fallback = widgets.find("shortcuts");
  return fallback != widgets.end() ? &fallback->second : nullptr;
}

std::vector<QuickShortcutsPanel::Entry> QuickShortcutsPanel::effectiveEntries() const {
  std::vector<Entry> entries;
  if (m_config == nullptr) {
    return entries;
  }

  const auto& shortcuts = m_config->config().shell.shortcuts;
  const WidgetConfig* config = widgetConfig();
  const std::string terminalGlyph = config != nullptr ? config->getString("terminal_glyph", "terminal-2") : "terminal-2";
  const std::string commandGlyph = config != nullptr ? config->getString("command_glyph", "player-play") : "player-play";
  const std::string appGlyph = config != nullptr ? config->getString("app_glyph", "apps") : "apps";
  entries.reserve(shortcuts.commands.size() + shortcuts.pinned.size());
  for (const auto& value : shortcuts.commands) {
    const auto parsed = quick_shortcuts::parseCommandEntry(value);
    if (!parsed.has_value()) {
      continue;
    }
    entries.push_back(
        Entry{
            .label = parsed->label,
            .glyph = parsed->terminal ? terminalGlyph : commandGlyph,
            .command = parsed->command,
            .terminal = parsed->terminal,
        }
    );
  }

  for (auto& desktopEntry : shell::dock::pinned_apps::resolveEntries(shortcuts.pinned)) {
    entries.push_back(
        Entry{
            .label = desktopEntry.name.empty() ? desktopEntry.id : desktopEntry.name,
            .glyph = appGlyph,
            .desktopEntry = std::move(desktopEntry),
        }
    );
  }
  return entries;
}

std::size_t QuickShortcutsPanel::entryCountForLayout() const {
  return m_entries.empty() ? effectiveEntries().size() : m_entries.size();
}

std::size_t QuickShortcutsPanel::columns() const {
  std::size_t configured = 4;
  if (const WidgetConfig* config = widgetConfig(); config != nullptr) {
    configured = static_cast<std::size_t>(std::clamp<std::int64_t>(config->getInt("columns", 4), 1, 6));
  }
  return std::min(configured, std::max<std::size_t>(1, entryCountForLayout()));
}

std::size_t QuickShortcutsPanel::rows() const {
  const std::size_t count = std::max<std::size_t>(1, entryCountForLayout());
  const std::size_t columnCount = columns();
  return (count + columnCount - 1) / columnCount;
}

float QuickShortcutsPanel::buttonWidth() const {
  const WidgetConfig* config = widgetConfig();
  return static_cast<float>(
      std::clamp<std::int64_t>(
          config != nullptr ? config->getInt("button_width", static_cast<std::int64_t>(kDefaultButtonWidth))
                            : static_cast<std::int64_t>(kDefaultButtonWidth),
          96, 240
      )
  );
}

float QuickShortcutsPanel::buttonHeight() const {
  const WidgetConfig* config = widgetConfig();
  return static_cast<float>(
      std::clamp<std::int64_t>(
          config != nullptr ? config->getInt("button_height", static_cast<std::int64_t>(kDefaultButtonHeight))
                            : static_cast<std::int64_t>(kDefaultButtonHeight),
          64, 180
      )
  );
}

float QuickShortcutsPanel::entryGlyphSize() const {
  const WidgetConfig* config = widgetConfig();
  return static_cast<float>(
      std::clamp<std::int64_t>(
          config != nullptr ? config->getInt("entry_glyph_size", static_cast<std::int64_t>(kDefaultGlyphSize))
                            : static_cast<std::int64_t>(kDefaultGlyphSize),
          16, 48
      )
  );
}

float QuickShortcutsPanel::preferredWidth() const {
  const std::size_t columnCount = columns();
  const float gap = Style::spaceXs;
  return scaled(
      buttonWidth() * static_cast<float>(columnCount)
      + gap * static_cast<float>(columnCount > 1 ? columnCount - 1 : 0)
      + Style::panelPadding * 2.0f
  );
}

float QuickShortcutsPanel::preferredHeight() const {
  const std::size_t rowCount = rows();
  const float gap = Style::spaceXs;
  return std::ceil(scaled(
      buttonHeight() * static_cast<float>(rowCount)
      + gap * static_cast<float>(rowCount > 1 ? rowCount - 1 : 0)
      + Style::panelPadding * 2.0f
  ));
}

void QuickShortcutsPanel::create() {
  m_entries = effectiveEntries();
  m_grid = nullptr;
  m_buttons.clear();

  if (m_entries.empty()) {
    auto empty = ui::column({
        .align = FlexAlign::Center,
        .justify = FlexJustify::Center,
        .minWidth = scaled(320.0f),
        .minHeight = scaled(buttonHeight()),
    });
    empty->addChild(
        ui::label({
            .text = i18n::tr("shortcuts.empty"),
            .fontSize = Style::fontSizeBody * contentScale(),
            .color = colorSpecFromRole(ColorRole::OnSurfaceVariant),
        })
    );
    setRoot(std::move(empty));
    return;
  }

  const float scale = contentScale();
  auto grid = std::make_unique<GridView>();
  grid->setColumns(columns());
  grid->setColumnGap(Style::spaceXs * scale);
  grid->setRowGap(Style::spaceXs * scale);
  grid->setStretchItems(true);
  grid->setUniformCellSize(true);
  grid->setMinCellWidth(buttonWidth() * scale);
  grid->setMinCellHeight(buttonHeight() * scale);
  m_grid = grid.get();

  m_buttons.reserve(m_entries.size());
  for (std::size_t i = 0; i < m_entries.size(); ++i) {
    if (Button* button = createEntryButton(m_entries[i], i, scale); button != nullptr) {
      m_buttons.push_back(button);
      grid->addChild(std::unique_ptr<Button>(button));
    }
  }
  setRoot(std::move(grid));
}

Button* QuickShortcutsPanel::createEntryButton(const Entry& entry, std::size_t index, float scale) {
  auto button = std::make_unique<Button>();
  button->setText(entry.label);
  button->setGlyph(entry.glyph);
  button->setVariant(ButtonVariant::Default);
  button->setSurfaceOpacity(panelCardOpacity());
  button->setDirection(FlexDirection::Vertical);
  button->setAlign(FlexAlign::Center);
  button->setJustify(FlexJustify::Center);
  button->setGap(Style::spaceXs * scale);
  button->setContentAlign(ButtonContentAlign::Center);
  button->setFontSize(Style::fontSizeMini * scale);
  button->setGlyphSize(entryGlyphSize() * scale);
  button->setPadding(Style::spaceSm * scale, Style::spaceSm * scale);
  button->setRadius(Style::scaledRadiusLg(scale));
  button->setMinWidth(buttonWidth() * scale);
  button->setMinHeight(buttonHeight() * scale);
  button->setFlexGrow(1.0f);
  button->setOnClick([this, index]() { activateEntry(index); });
  return button.release();
}

void QuickShortcutsPanel::activateEntry(std::size_t index) {
  if (index >= m_entries.size()) {
    return;
  }
  const Entry entry = m_entries[index];
  PanelManager::instance().close();

  if (entry.desktopEntry.has_value()) {
    desktop_entry_launch::LaunchOptions options;
    if (m_config != nullptr) {
      options.runAsSystemdService = m_config->config().shell.launchAppsAsSystemdServices;
      options.customCommand = m_config->config().shell.launchAppsCustomCommand;
    }
    if (!desktop_entry_launch::launchEntry(*entry.desktopEntry, options)) {
      kLog.warn("could not launch pinned desktop entry \"{}\"", entry.desktopEntry->id);
    }
    return;
  }

  const bool launched = entry.terminal ? terminal_launch::launch(entry.command) : process::runAsync(entry.command);
  if (!launched) {
    kLog.warn("could not launch shortcut \"{}\"", entry.label);
  }
}

void QuickShortcutsPanel::doLayout(Renderer& renderer, float width, float height) {
  if (root() == nullptr) {
    return;
  }
  root()->setSize(width, height);
  root()->layout(renderer);
  for (Button* button : m_buttons) {
    if (button != nullptr) {
      button->updateInputArea();
    }
  }
}
