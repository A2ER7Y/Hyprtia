#include "shell/shortcuts/quick_shortcut_entry.h"

#include "util/string_utils.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>

namespace quick_shortcuts {
  namespace {

    [[nodiscard]] std::string derivedLabel(std::string_view command) {
      std::string token = StringUtils::trim(command);
      if (token.empty()) {
        return {};
      }

      if (const auto whitespace = token.find_first_of(" \t"); whitespace != std::string::npos) {
        token.resize(whitespace);
      }
      if (token.size() >= 2
          && ((token.front() == '"' && token.back() == '"') || (token.front() == '\'' && token.back() == '\''))) {
        token = token.substr(1, token.size() - 2);
      }
      if (const auto slash = token.find_last_of('/'); slash != std::string::npos && slash + 1 < token.size()) {
        token = token.substr(slash + 1);
      }
      if (token.empty()) {
        return std::string(command);
      }
      token.front() = static_cast<char>(std::toupper(static_cast<unsigned char>(token.front())));
      return token;
    }

  } // namespace

  std::optional<CommandEntry> parseCommandEntry(std::string_view value) {
    std::string source = StringUtils::trim(value);
    if (source.empty()) {
      return std::nullopt;
    }

    std::string label;
    std::string command = source;
    if (const auto separator = source.find("::"); separator != std::string::npos) {
      label = StringUtils::trim(source.substr(0, separator));
      command = StringUtils::trim(source.substr(separator + 2));
    }

    bool terminal = false;
    constexpr std::string_view kTerminalPrefix = "terminal:";
    const std::string commandLower = StringUtils::toLower(command);
    if (commandLower.starts_with(kTerminalPrefix)) {
      terminal = true;
      command = StringUtils::trim(command.substr(kTerminalPrefix.size()));
    }
    if (command.empty()) {
      return std::nullopt;
    }
    if (label.empty()) {
      label = derivedLabel(command);
    }

    return CommandEntry{
        .label = std::move(label),
        .command = std::move(command),
        .terminal = terminal,
    };
  }

} // namespace quick_shortcuts
