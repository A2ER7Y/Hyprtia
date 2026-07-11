#include "shell/shortcuts/quick_shortcut_entry.h"

#include <cassert>

int main() {
  {
    const auto entry = quick_shortcuts::parseCommandEntry("Install dotfiles :: terminal: sh ~/dotfiles/install.sh");
    assert(entry.has_value());
    assert(entry->label == "Install dotfiles");
    assert(entry->command == "sh ~/dotfiles/install.sh");
    assert(entry->terminal);
  }

  {
    const auto entry = quick_shortcuts::parseCommandEntry("/usr/bin/foot --server");
    assert(entry.has_value());
    assert(entry->label == "Foot");
    assert(entry->command == "/usr/bin/foot --server");
    assert(!entry->terminal);
  }

  {
    const auto entry = quick_shortcuts::parseCommandEntry("Browser :: chromium");
    assert(entry.has_value());
    assert(entry->label == "Browser");
    assert(entry->command == "chromium");
    assert(!entry->terminal);
  }

  assert(!quick_shortcuts::parseCommandEntry("   ").has_value());
  assert(!quick_shortcuts::parseCommandEntry("Broken :: terminal:   ").has_value());
  return 0;
}
