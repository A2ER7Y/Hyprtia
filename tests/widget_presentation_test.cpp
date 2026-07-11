#include "shell/bar/widget_presentation.h"

#include <cassert>
#include <string>

int main() {
  assert(widget_presentation::shouldConstruct(nullptr));
  WidgetConfig disabled;
  disabled.settings["enabled"] = false;
  assert(!widget_presentation::shouldConstruct(&disabled));

  WidgetConfig medium;
  medium.settings["presentation"] = std::string("medium");
  medium.settings["show_label"] = true;
  widget_presentation::apply(medium, "volume");
  assert(medium.getBool("show_label", false));
  assert(widget_presentation::mode(medium) == widget_presentation::Mode::Medium);

  WidgetConfig volume;
  volume.settings["presentation"] = std::string("small");
  volume.settings["show_label"] = true;
  widget_presentation::apply(volume, "volume");
  assert(!volume.getBool("show_label", true));

  WidgetConfig notifications;
  notifications.settings["presentation"] = std::string("small");
  notifications.settings["display_mode"] = std::string("icons");
  widget_presentation::apply(notifications, "notifications");
  assert(notifications.getString("display_mode", "") == "bell");

  WidgetConfig media;
  media.settings["presentation"] = std::string("small");
  widget_presentation::apply(media, "media");
  assert(media.getBool("album_art_only", false));
  assert(media.getBool("hide_artist", false));

  WidgetConfig unknown;
  unknown.settings["presentation"] = std::string("small");
  unknown.settings["custom"] = std::string("unchanged");
  widget_presentation::apply(unknown, "plugin.example");
  assert(unknown.getString("custom", "") == "unchanged");
  return 0;
}
