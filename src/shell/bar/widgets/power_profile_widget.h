#pragma once

#include "shell/bar/widget.h"

#include <string>

class Glyph;
class InputArea;
class Label;
class PowerProfilesService;

class PowerProfileWidget : public Widget {
public:
  enum class StateDisplay {
    PerformanceStatus,
    ProfileName,
  };

  struct Options {
    bool showState = false;
    StateDisplay stateDisplay = StateDisplay::PerformanceStatus;
    bool hideWhenUnavailable = false;
  };

  explicit PowerProfileWidget(PowerProfilesService* powerProfiles, Options options);

  void create() override;

private:
  void doLayout(Renderer& renderer, float containerWidth, float containerHeight) override;
  void doUpdate(Renderer& renderer) override;
  void syncState(Renderer& renderer);
  void cycleProfile(int direction);

  PowerProfilesService* m_powerProfiles = nullptr;
  InputArea* m_area = nullptr;
  Glyph* m_glyph = nullptr;
  Label* m_stateLabel = nullptr;
  std::string m_lastProfile;
  Options m_options;
  bool m_haveState = false;
  bool m_available = false;
  bool m_performanceActive = false;
};
