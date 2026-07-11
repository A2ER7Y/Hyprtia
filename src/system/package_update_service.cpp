#include "system/package_update_service.h"

#include "core/build_info.h"
#include "core/process/process.h"
#include "util/string_utils.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace {

  constexpr auto kPollInterval = std::chrono::minutes(30);
  constexpr auto kCommandTimeout = std::chrono::seconds(20);
  constexpr std::size_t kMaxOutputBytes = 512 * 1024;
  constexpr std::size_t kMaxAppImages = 40;

  std::string envOr(std::string_view name, std::string_view fallback) {
    const char* value = std::getenv(std::string(name).c_str());
    return value != nullptr && *value != '\0' ? std::string(value) : std::string(fallback);
  }

  bool bedrockRoutingEnabled() {
    const char* forced = std::getenv("HYPRTIA_FORCE_BEDROCK");
    return process::commandExists("strat")
        && ((forced != nullptr && std::string_view(forced) == "1")
            || std::filesystem::exists("/bedrock/etc/bedrock-release"));
  }

  process::RunOptions runOptions(const std::shared_ptr<std::atomic<bool>>& cancel) {
    return {
        .timeout = kCommandTimeout,
        .maxOutputBytes = kMaxOutputBytes,
        .cancel = cancel,
    };
  }

  std::vector<std::string_view> nonEmptyLines(std::string_view text) {
    std::vector<std::string_view> lines;
    while (!text.empty()) {
      const std::size_t newline = text.find('\n');
      std::string_view line = text.substr(0, newline);
      while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t')) {
        line.remove_suffix(1);
      }
      while (!line.empty() && (line.front() == ' ' || line.front() == '\t')) {
        line.remove_prefix(1);
      }
      if (!line.empty()) {
        lines.push_back(line);
      }
      if (newline == std::string_view::npos) {
        break;
      }
      text.remove_prefix(newline + 1);
    }
    return lines;
  }

} // namespace

int PackageUpdateSnapshot::total() const noexcept {
  int result = 0;
  for (const int count : {arch, aur, fedora, flatpak, snap, appimage, hyprtia}) {
    result += std::max(0, count);
  }
  return result;
}

namespace package_updates {

  int countNonEmptyLines(std::string_view text) {
    return static_cast<int>(
        std::min<std::size_t>(nonEmptyLines(text).size(), static_cast<std::size_t>(std::numeric_limits<int>::max()))
    );
  }

  int countDnfUpdates(std::string_view text) {
    int count = 0;
    for (const std::string_view line : nonEmptyLines(text)) {
      if (line.starts_with("Updating and loading repositories")
          || line.starts_with("Repositories loaded")
          || line.starts_with("Last metadata expiration check")
          || line.starts_with("Available upgrades")) {
        continue;
      }
      ++count;
    }
    return count;
  }

  int countSnapUpdates(std::string_view text) {
    const auto lines = nonEmptyLines(text);
    if (lines.empty()) {
      return 0;
    }
    if (lines.front().starts_with("All snaps up to date")) {
      return 0;
    }
    const bool hasHeader = lines.front().starts_with("Name ") || lines.front().starts_with("Name\t");
    return static_cast<int>(lines.size() - (hasHeader ? 1 : 0));
  }

  std::string extractGitHubCommitSha(std::string_view json) {
    const std::size_t key = json.find("\"sha\"");
    if (key == std::string_view::npos) {
      return {};
    }
    const std::size_t colon = json.find(':', key + 5);
    const std::size_t quote = colon == std::string_view::npos ? colon : json.find('"', colon + 1);
    const std::size_t end = quote == std::string_view::npos ? quote : json.find('"', quote + 1);
    if (quote == std::string_view::npos || end == std::string_view::npos || end <= quote + 1) {
      return {};
    }
    return std::string(json.substr(quote + 1, end - quote - 1));
  }

  bool revisionDiffers(std::string_view localRevision, std::string_view remoteRevision) {
    if (localRevision.empty() || remoteRevision.empty() || localRevision == "unknown") {
      return false;
    }
    const std::size_t commonLength = std::min(localRevision.size(), remoteRevision.size());
    return localRevision.substr(0, commonLength) != remoteRevision.substr(0, commonLength);
  }

} // namespace package_updates

PackageUpdateService::PackageUpdateService() : m_cancel(std::make_shared<std::atomic<bool>>(false)) {}

PackageUpdateService::~PackageUpdateService() { stop(); }

void PackageUpdateService::start() {
  std::scoped_lock lock(m_mutex);
  if (m_worker.joinable()) {
    return;
  }
  m_cancel = std::make_shared<std::atomic<bool>>(false);
  m_refreshRequested = true;
  m_worker = std::jthread([this](std::stop_token token) { workerLoop(token); });
}

void PackageUpdateService::stop() {
  {
    std::scoped_lock lock(m_mutex);
    if (!m_worker.joinable()) {
      return;
    }
    m_cancel->store(true);
    m_worker.request_stop();
    m_wake.notify_all();
  }
  m_worker.join();
}

void PackageUpdateService::refreshNow() {
  std::scoped_lock lock(m_mutex);
  m_refreshRequested = true;
  m_wake.notify_all();
}

void PackageUpdateService::setChangeCallback(ChangeCallback callback) {
  std::scoped_lock lock(m_mutex);
  m_changeCallback = std::move(callback);
}

PackageUpdateSnapshot PackageUpdateService::snapshot() const {
  std::scoped_lock lock(m_mutex);
  return m_snapshot;
}

void PackageUpdateService::workerLoop(std::stop_token stopToken) {
  while (!stopToken.stop_requested()) {
    {
      std::scoped_lock lock(m_mutex);
      m_refreshRequested = false;
    }

    PackageUpdateSnapshot checking = snapshot();
    checking.checking = true;
    publish(std::move(checking));

    PackageUpdateSnapshot next = checkAll();
    if (stopToken.stop_requested()) {
      break;
    }
    next.checking = false;
    publish(std::move(next));

    std::unique_lock lock(m_mutex);
    m_wake.wait_for(lock, stopToken, kPollInterval, [this]() { return m_refreshRequested; });
  }
}

PackageUpdateSnapshot PackageUpdateService::checkAll() const {
  PackageUpdateSnapshot result;
  auto run = [this](const std::vector<std::string>& args) { return process::runSync(args, runOptions(m_cancel)); };
  const bool useBedrock = bedrockRoutingEnabled();
  const std::string systemStratum = envOr("HYPRTIA_SYSTEM_STRATUM", "kirolinux");
  const std::string appsStratum = envOr("HYPRTIA_APPS_STRATUM", "fedora");
  auto routedArgs = [useBedrock](const std::string& stratum, std::vector<std::string> args) {
    if (!useBedrock) {
      return args;
    }
    args.insert(args.begin(), stratum);
    args.insert(args.begin(), "-r");
    args.insert(args.begin(), "strat");
    return args;
  };
  auto available = [&](const std::string& stratum, const std::string& command) {
    if (!useBedrock) {
      return process::commandExists(command);
    }
    const auto check = run({"strat", "-r", stratum, "sh", "-c", "command -v \"$1\" >/dev/null 2>&1", "sh", command});
    return !check.timedOut && check.exitCode == 0;
  };
  auto runIn = [&](const std::string& stratum, std::vector<std::string> args) {
    return run(routedArgs(stratum, std::move(args)));
  };

  if (available(systemStratum, "checkupdates")) {
    const auto check = runIn(systemStratum, {"checkupdates"});
    if (!check.timedOut) {
      result.arch = 0;
      for (const std::string_view line : nonEmptyLines(check.out)) {
        if (line.starts_with("hyprtia ") || line.starts_with("hyprtia\t")) {
          result.hyprtia = 1;
        } else {
          ++result.arch;
        }
      }
    }
  }

  const std::string aurHelper =
      available(systemStratum, "yay") ? "yay" : (available(systemStratum, "paru") ? "paru" : "");
  if (!aurHelper.empty()) {
    const auto check = runIn(systemStratum, {aurHelper, "-Qua"});
    if (!check.timedOut) {
      result.aur = package_updates::countNonEmptyLines(check.out);
    }
  }

  const std::string dnfCommand = available(appsStratum, "dnf5") ? "dnf5" : (available(appsStratum, "dnf") ? "dnf" : "");
  if (!dnfCommand.empty()) {
    const auto check =
        runIn(appsStratum, {dnfCommand, "-q", "repoquery", "--upgrades", "--queryformat=%{name}.%{arch}"});
    if (!check.timedOut && check.exitCode == 0) {
      result.fedora = package_updates::countDnfUpdates(check.out);
    }
  }

  if (available(appsStratum, "flatpak")) {
    const auto check = runIn(appsStratum, {"flatpak", "remote-ls", "--updates", "--app", "--columns=application"});
    if (!check.timedOut) {
      result.flatpak = package_updates::countNonEmptyLines(check.out);
    }
  }

  if (available(appsStratum, "snap")) {
    const auto check = runIn(appsStratum, {"snap", "refresh", "--list"});
    if (!check.timedOut) {
      result.snap = package_updates::countSnapUpdates(check.out);
    }
  }

  if (available(appsStratum, "appimageupdatetool")) {
    result.appimage = 0;
    const char* home = std::getenv("HOME");
    const std::filesystem::path appDir =
        home != nullptr ? std::filesystem::path(home) / "Applications" : std::filesystem::path{};
    std::error_code ec;
    std::size_t checked = 0;
    if (!appDir.empty() && std::filesystem::is_directory(appDir, ec)) {
      for (const auto& entry : std::filesystem::directory_iterator(appDir, ec)) {
        if (m_cancel->load() || checked >= kMaxAppImages) {
          break;
        }
        if (!entry.is_regular_file(ec) || StringUtils::toLower(entry.path().extension().string()) != ".appimage") {
          continue;
        }
        ++checked;
        const auto check = runIn(appsStratum, {"appimageupdatetool", "--check-for-update", entry.path().string()});
        if (!check.timedOut && check.exitCode == 1) {
          ++result.appimage;
        }
      }
    }
  }

  if (result.hyprtia != 1 && available(systemStratum, "curl")) {
    const auto check = runIn(
        systemStratum,
        {"curl", "-fsSL", "--max-time", "15", "-H", "Accept: application/vnd.github+json",
         "https://api.github.com/repos/A2ER7Y/Hyprtia/commits/main"}
    );
    const std::string remoteRevision = package_updates::extractGitHubCommitSha(check.out);
    const std::string_view localRevision = noctalia::build_info::revision();
    if (!check.timedOut && !remoteRevision.empty() && !localRevision.empty() && localRevision != "unknown") {
      result.hyprtia = package_updates::revisionDiffers(localRevision, remoteRevision) ? 1 : 0;
    }
  }

  return result;
}

void PackageUpdateService::publish(PackageUpdateSnapshot next) {
  ChangeCallback callback;
  {
    std::scoped_lock lock(m_mutex);
    next.revision = m_snapshot.revision + 1;
    m_snapshot = std::move(next);
    callback = m_changeCallback;
  }
  if (callback) {
    callback();
  }
}
