#include "notification/notification_manager.h"

#include <cassert>
#include <chrono>
#include <deque>
#include <string>

namespace {

  NotificationHistoryEntry entry(std::uint32_t id, std::string appName, std::string desktopEntry = {}) {
    Notification notification{
        .id = id,
        .origin = NotificationOrigin::External,
        .appName = std::move(appName),
        .summary = "summary",
        .body = {},
        .timeout = 0,
        .urgency = Urgency::Normal,
        .desktopEntry = desktopEntry.empty() ? std::nullopt : std::optional<std::string>{std::move(desktopEntry)},
        .receivedTime = Clock::now(),
    };
    return NotificationHistoryEntry{.notification = std::move(notification), .eventSerial = id};
  }

} // namespace

int main() {
  std::deque<NotificationHistoryEntry> history;
  for (std::uint32_t i = 1; i <= 12; ++i) {
    history.push_back(entry(i, "App " + std::to_string(i), "app" + std::to_string(i)));
  }
  // The newest duplicate must win without consuming another icon slot.
  history.push_back(entry(13, "Renamed App 12", "app12"));

  const auto recent = collectRecentNotificationApps(history, 11);
  assert(recent.size() == 11);
  assert(recent.front().displayName == "Renamed App 12");
  assert(recent.front().key == "desktop:app12");
  assert(recent[1].displayName == "App 11");
  assert(recent.back().displayName == "App 2");

  const auto capped = collectRecentNotificationApps(history, 10);
  assert(capped.size() == 10);
  assert(capped.back().displayName == "App 3");
  assert(collectRecentNotificationApps(history, 0).empty());
  return 0;
}
