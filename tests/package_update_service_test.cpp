#include "system/package_update_service.h"

#include <cassert>

int main() {
  using namespace package_updates;

  assert(countNonEmptyLines("") == 0);
  assert(countNonEmptyLines("\n  \n") == 0);
  assert(countNonEmptyLines("one\ntwo\nthree\n") == 3);

  assert(countSnapUpdates("All snaps up to date.\n") == 0);
  assert(countSnapUpdates("Name Version Rev Size Publisher Notes\nfirefox 1 2 3 canonical** -\n") == 1);
  assert(countSnapUpdates("firefox 1 2 3 canonical** -\n") == 1);

  assert(extractGitHubCommitSha(R"({"sha":"abcdef1234567890","node_id":"x"})") == "abcdef1234567890");
  assert(extractGitHubCommitSha("{}").empty());
  assert(!revisionDiffers("abcdef123456", "abcdef1234567890"));
  assert(revisionDiffers("abcdef123456", "0000001234567890"));
  assert(!revisionDiffers("unknown", "abcdef1234567890"));

  PackageUpdateSnapshot snapshot{
      .arch = 2,
      .aur = 3,
      .flatpak = 1,
      .snap = -1,
      .appimage = 4,
      .hyprtia = 1,
  };
  assert(snapshot.total() == 11);
  return 0;
}
