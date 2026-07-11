#pragma once

#include "shell/bar/widget.h"
#include "system/package_update_service.h"

#include <string>

class Glyph;
class InputArea;
class Label;

class UpdatesWidget : public Widget {
public:
  struct Options {
    bool hideWhenZero = false;
    bool showLabel = true;
    std::string glyph = "download";
    std::string command;
  };

  UpdatesWidget(PackageUpdateService* updates, Options options);

  void create() override;

private:
  void doLayout(Renderer& renderer, float containerWidth, float containerHeight) override;
  void doUpdate(Renderer& renderer) override;
  void syncState();
  [[nodiscard]] std::string tooltipText(const PackageUpdateSnapshot& state) const;

  PackageUpdateService* m_updates = nullptr;
  Options m_options;
  InputArea* m_area = nullptr;
  Glyph* m_glyph = nullptr;
  Label* m_label = nullptr;
  PackageUpdateSnapshot m_lastState;
  bool m_haveState = false;
};
