#include "shell/bar/widget_presentation.h"

#include <string>
#include <utility>

namespace widget_presentation {

  Mode mode(const WidgetConfig& config) {
    return config.getString("presentation", "medium") == "small" ? Mode::Small : Mode::Medium;
  }

  bool shouldConstruct(const WidgetConfig* config) { return config == nullptr || config->getBool("enabled", true); }

  void apply(WidgetConfig& config, std::string_view type) {
    if (mode(config) != Mode::Small) {
      return;
    }

    auto setBool = [&config](std::string key, bool value) { config.settings.insert_or_assign(std::move(key), value); };
    auto setString = [&config](std::string key, std::string value) {
      config.settings.insert_or_assign(std::move(key), std::move(value));
    };

    if (type == "active_window") {
      setString("display", "icon_only");
    } else if (type == "battery") {
      setString("display_mode", "glyph");
      setBool("show_label", false);
    } else if (
        type == "bluetooth" || type == "brightness" || type == "network" || type == "updates" || type == "volume"
    ) {
      setBool("show_label", false);
    } else if (type == "keyboard_layout") {
      setString("display", "short");
      setBool("show_icon", true);
      setBool("show_label", false);
    } else if (type == "lock_keys") {
      setString("display", "short");
    } else if (type == "media") {
      setBool("album_art_only", true);
      setBool("hide_artist", true);
    } else if (type == "notifications") {
      setString("display_mode", "bell");
    } else if (type == "power_profile") {
      setBool("show_state", false);
    } else if (type == "sysmon") {
      setString("display", "gauge");
      setBool("show_label", false);
    } else if (type == "weather") {
      setBool("show_condition", false);
      setBool("show_temperature", false);
    }
  }

} // namespace widget_presentation
