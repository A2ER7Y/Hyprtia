#include "notification/notification_icon_resolver.h"

#include "net/uri.h"
#include "notification/notification.h"
#include "system/icon_resolver.h"

#include <filesystem>
#include <functional>
#include <unistd.h>

namespace {

  std::filesystem::path remoteNotificationIconCachePath(std::string_view url) {
    return std::filesystem::path("/tmp")
        / "noctalia-notification-icons"
        / (std::to_string(std::hash<std::string_view>{}(url)) + ".img");
  }

  std::string resolveIconValue(std::string_view iconValue, IconResolver& resolver, int targetSize) {
    if (iconValue.empty()) {
      return {};
    }
    if (uri::isRemoteUrl(iconValue)) {
      const auto cached = remoteNotificationIconCachePath(iconValue);
      std::error_code ec;
      if (std::filesystem::exists(cached, ec) && std::filesystem::file_size(cached, ec) > 0) {
        return cached.string();
      }
      return {};
    }

    const std::string localPath = uri::normalizeFileUrl(iconValue);
    if (!localPath.empty() && localPath.front() == '/') {
      return access(localPath.c_str(), R_OK) == 0 ? localPath : std::string{};
    }
    if (localPath.empty()) {
      return {};
    }
    return resolver.resolve(localPath, targetSize);
  }

} // namespace

std::string resolveNotificationIconPath(
    const std::optional<std::string>& icon, const std::optional<std::string>& desktopEntry, IconResolver& resolver,
    int targetSize
) {
  if (icon.has_value()) {
    if (std::string path = resolveIconValue(*icon, resolver, targetSize); !path.empty()) {
      return path;
    }
  }
  if (desktopEntry.has_value()) {
    return resolveIconValue(*desktopEntry, resolver, targetSize);
  }
  return {};
}

std::string resolveNotificationIconPath(const Notification& notification, IconResolver& resolver, int targetSize) {
  return resolveNotificationIconPath(notification.icon, notification.desktopEntry, resolver, targetSize);
}
