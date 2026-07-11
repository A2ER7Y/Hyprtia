#pragma once

#include <optional>
#include <string>
#include <string_view>

class IconResolver;
struct Notification;

[[nodiscard]] std::string
resolveNotificationIconPath(const Notification& notification, IconResolver& resolver, int targetSize);

[[nodiscard]] std::string resolveNotificationIconPath(
    const std::optional<std::string>& icon, const std::optional<std::string>& desktopEntry, IconResolver& resolver,
    int targetSize
);
