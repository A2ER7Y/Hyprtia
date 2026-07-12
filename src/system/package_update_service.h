#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <stop_token>
#include <string>
#include <string_view>
#include <thread>

struct PackageUpdateSnapshot {
  // -1 means that the corresponding package manager/checker is unavailable.
  int arch = -1;
  int aur = -1;
  int fedora = -1;
  int flatpak = -1;
  int snap = -1;
  int appimage = -1;
  int hyprtia = -1;
  bool checking = false;
  std::uint64_t revision = 0;

  [[nodiscard]] int total() const noexcept;
  bool operator==(const PackageUpdateSnapshot&) const = default;
};

class PackageUpdateService {
public:
  using ChangeCallback = std::function<void()>;

  PackageUpdateService();
  ~PackageUpdateService();

  PackageUpdateService(const PackageUpdateService&) = delete;
  PackageUpdateService& operator=(const PackageUpdateService&) = delete;

  void start();
  void stop();
  void refreshNow();
  void setChangeCallback(ChangeCallback callback);
  [[nodiscard]] PackageUpdateSnapshot snapshot() const;

private:
  void workerLoop(std::stop_token stopToken);
  [[nodiscard]] PackageUpdateSnapshot checkAll() const;
  void publish(PackageUpdateSnapshot next);

  mutable std::mutex m_mutex;
  std::condition_variable_any m_wake;
  PackageUpdateSnapshot m_snapshot;
  ChangeCallback m_changeCallback;
  std::jthread m_worker;
  std::shared_ptr<std::atomic<bool>> m_cancel;
  bool m_refreshRequested = false;
};

namespace package_updates {
  [[nodiscard]] int countNonEmptyLines(std::string_view text);
  [[nodiscard]] int countDnfUpdates(std::string_view text);
  [[nodiscard]] int countSnapUpdates(std::string_view text);
  [[nodiscard]] std::string extractGitHubCommitSha(std::string_view json);
  [[nodiscard]] bool revisionDiffers(std::string_view localRevision, std::string_view remoteRevision);
} // namespace package_updates
