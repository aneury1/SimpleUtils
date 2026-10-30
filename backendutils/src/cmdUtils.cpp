#include <simpleutils/cmdUtils.h>
#include <simpleutils/encryptionUtils.h>
#include <simpleutils/functionUtils.h>
#include <simpleutils/stringUtils.h>

#include <cstdlib>
#include <filesystem>
#include <stdexcept>
#ifndef _WIN32
#include <sys/wait.h>
#include <unistd.h>
#endif
namespace simpleutils {
namespace fs = std::filesystem;
static std::string resolveProgram(std::string name) {
  if (name.empty()) return "";
  if (name.find_first_of("/\\") != name.npos) {
    std::error_code ec;
    if (!fs::is_regular_file(name, ec)) return "";
#ifndef _WIN32
    if (access(name.c_str(), X_OK) != 0) return "";
#endif
    return fs::absolute(name).string();
  }
  const char* path = std::getenv("PATH");
  if (!path) return "";
#ifdef _WIN32
  auto dirs = split(path, ";");
#else
  auto dirs = split(path, ":");
#endif
  for (const auto& d : dirs) {
    auto p = fs::path(d.empty() ? "." : d) / name;
#ifdef _WIN32
    if (!p.has_extension()) p += ".exe";
#endif
    std::error_code ec;
    if (!fs::is_regular_file(p, ec)) continue;
#ifndef _WIN32
    if (access(p.c_str(), X_OK) != 0) continue;
#endif
    return fs::absolute(p).string();
  }
  return "";
}
bool IsInstalled(const char* p) { return p && !resolveProgram(p).empty(); }
TTYTerminal GetTerminal() {
#ifdef _WIN32
  return TTYTerminal::WindowCMD;
#elif defined(__APPLE__)
  return TTYTerminal::MacOS;
#else
  if (IsInstalled("konsole")) return TTYTerminal::Konsole;
  if (IsInstalled("gnome-terminal")) return TTYTerminal::Gnome;
  return TTYTerminal::OtherLinux;
#endif
}
std::string getLatestClangFormat() {
  for (int i = 30; i >= 11; --i) {
    auto name = "clang-format-" + std::to_string(i);
    if (IsInstalled(name.c_str())) return name;
  }
  return IsInstalled("clang-format") ? "clang-format" : "";
}
std::string formatWithClangFormat(const std::string& code, const std::string& executable) {
  auto program = resolveProgram(executable);
  if (program.empty()) throw std::invalid_argument("Formatter executable not found");
#ifdef _WIN32
  // Shell-free process creation is deliberately not emulated using system().
  throw std::runtime_error("formatWithClangFormat currently requires POSIX");
#else
  std::string path = (fs::temp_directory_path() / "simpleutils-format-XXXXXX").string();
  std::vector<char> chars(path.begin(), path.end());
  chars.push_back('\0');
  int fd = mkstemp(chars.data());
  if (fd < 0) throw std::runtime_error("Cannot create formatter temporary file");
  ::close(fd);
  path = chars.data();
  auto cleanup = scopeExit([&] {
    std::error_code ec;
    fs::remove(path, ec);
  });
  WriteFile(path, code);
  pid_t pid = fork();
  if (pid < 0) throw std::runtime_error("fork failed");
  if (!pid) {
    execl(program.c_str(), program.c_str(), "-i", path.c_str(), static_cast<char*>(nullptr));
    _exit(127);
  }
  int status = 0;
  while (waitpid(pid, &status, 0) < 0) {
    if (errno != EINTR) throw std::runtime_error("waitpid failed");
  }
  if (!WIFEXITED(status) || WEXITSTATUS(status)) throw std::runtime_error("Formatter failed");
  return ReadFile(path);
#endif
}
}  // namespace simpleutils
