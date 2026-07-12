#pragma once

#include "config/config_types.h"

#include <string_view>

namespace widget_presentation {

  enum class Mode {
    Small,
    Medium,
  };

  [[nodiscard]] Mode mode(const WidgetConfig& config);
  [[nodiscard]] bool shouldConstruct(const WidgetConfig* config);
  void apply(WidgetConfig& config, std::string_view type);

} // namespace widget_presentation
