#include "notification/notification_manager.h"
#include "util/string_utils.h"

#include <algorithm>
#include <unordered_set>

std::vector<RecentNotificationApp>
collectRecentNotificationApps(const std::deque<NotificationHistoryEntry>& history, std::size_t maxApps) {
  std::vector<RecentNotificationApp> apps;
  if (maxApps == 0) {
    return apps;
  }

  apps.reserve(std::min(maxApps, history.size()));
  std::unordered_set<std::string> seen;
  for (auto it = history.rbegin(); it != history.rend() && apps.size() < maxApps; ++it) {
    const Notification& notification = it->notification;
    const std::string desktopEntry =
        notification.desktopEntry.has_value() ? StringUtils::trim(*notification.desktopEntry) : std::string{};
    const std::string appName = StringUtils::trim(notification.appName);
    const std::string identity = !desktopEntry.empty() ? "desktop:" + StringUtils::toLower(desktopEntry)
                                                       : "app:" + StringUtils::toLower(appName);
    if (!seen.insert(identity).second) {
      continue;
    }
    apps.push_back(
        RecentNotificationApp{
            .key = identity,
            .displayName = appName.empty() ? std::string{"System"} : appName,
            .icon = notification.icon,
            .desktopEntry = notification.desktopEntry,
        }
    );
  }
  return apps;
}
