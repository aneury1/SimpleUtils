#pragma once
#include <uiutils/DefaultThemes.h>
#include <memory>
#include <uiutils/layout.hpp>
namespace uiutils {
namespace detail {
struct ActivityState;
struct ModuleState;
}  // namespace detail
enum class WindowIdCode : int { ID_LOAD_MODULE = 10000 };
class ModuleActivity;
class ActivityFrame {
  std::unique_ptr<detail::ActivityState> state_;
  friend class ModuleActivity;
  friend class UIApplication;

 protected:
  void AddModule(std::unique_ptr<ModuleActivity> module);

 public:
  explicit ActivityFrame(const std::string& title = "uiutils",
                         Size size = Size(1100, 800));
  virtual ~ActivityFrame();
  ActivityFrame(const ActivityFrame&) = delete;
  ActivityFrame& operator=(const ActivityFrame&) = delete;
  virtual void OnCreate() {}
  void SetTheme(Theme theme);
  int addMenuBarOption(const std::string& label, int action = AutoId);
  int addMenuBarOption(const std::string& label, const std::string& actionLabel,
                       int action = AutoId);
  void OnMenu(int action, std::function<void()> callback);
  void AddExitMenu(const std::string& label = "&File");
  void Show();
  void Close();
  template <class T, class... Args>
  void OpenModule(Args&&... args) {
    auto module = std::make_unique<T>(this, std::forward<Args>(args)...);
    module->OnCreate();
    module->Show();
    AddModule(std::move(module));
  }
};
class ModuleActivity {
  std::unique_ptr<detail::ModuleState> state_;
  friend class ActivityFrame;

 public:
  ModuleActivity(ActivityFrame* parent, const std::string& title,
                 Size size = Size(900, 650));
  virtual ~ModuleActivity();
  ModuleActivity(const ModuleActivity&) = delete;
  ModuleActivity& operator=(const ModuleActivity&) = delete;
  virtual void OnCreate() {}
  void SetContent(const LayoutScope::Builder& build);
  void Show();
  void Close();
  bool IsOpen() const;
  void Message(const std::string& text, const std::string& title = "Message");
  bool Confirm(const std::string& text, const std::string& title = "Confirm");
  void PreviewText(const std::string& text,
                   const std::string& title = "Preview");
};
}  // namespace uiutils
