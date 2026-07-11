#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace quick_shortcuts {

  struct CommandEntry {
    std::string label;
    std::string command;
    bool terminal = false;

    bool operator==(const CommandEntry&) const = default;
  };

  // Parses "Label :: command" entries. Prefixing the command with "terminal:"
  // runs it through the configured/default terminal.
  [[nodiscard]] std::optional<CommandEntry> parseCommandEntry(std::string_view value);

} // namespace quick_shortcuts
