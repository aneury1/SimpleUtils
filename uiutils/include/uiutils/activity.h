#pragma once
#include <uiutils/layout.hpp>
#include <uiutils/DefaultThemes.h>
#include <memory>
namespace uiutils {
namespace detail { struct ActivityState; struct ModuleState; }
enum class WindowIdCode : int { ID_LOAD_MODULE = 10000 };
class ModuleActivity;
class ActivityFrame {
    std::unique_ptr<detail::ActivityState> state_;
    friend class ModuleActivity;
    friend class UIApplication;
protected:
    void AddModule(std::unique_ptr<ModuleActivity> module);
public:
    explicit ActivityFrame(const std::string& title = "uiutils", Size size = Size(1100, 800));
    virtual ~ActivityFrame();
    ActivityFrame(const ActivityFrame&) = delete;
    ActivityFrame& operator=(const ActivityFrame&) = delete;
    virtual void OnCreate() {}
    // Factories must return a module belonging to this frame.
    using FrameFactory = std::function<std::unique_ptr<ModuleActivity>(ActivityFrame*)>;
    void RegisterFrame(const std::string& name, FrameFactory factory);
    template<class T> void RegisterFrame(const std::string& name) {
        RegisterFrame(name, [](ActivityFrame* parent) { return std::make_unique<T>(parent); });
    }
    void Navigate(const std::string& name);
    bool GoBack();
    bool CanGoBack() const;
    void CloseFrame(const std::string& name);
    // Non-owning; do not retain across closing/opening modules.
    ModuleActivity* GetCurrentFrame() const;
    void SetTheme(Theme theme);
    int addMenuBarOption(const std::string& label, int action = AutoId);
    int addMenuBarOption(const std::string& label, const std::string& actionLabel, int action = AutoId);
    void OnMenu(int action, std::function<void()> callback);
    void AddExitMenu(const std::string& label = "&File");
    void Show();
    void Close();
    template<class T, class... Args> void OpenModule(Args&&... args) {
        auto module = std::make_unique<T>(this, std::forward<Args>(args)...);
        module->OnCreate(); module->Show(); AddModule(std::move(module));
    }
};
class ModuleActivity {
    std::unique_ptr<detail::ModuleState> state_;
    friend class ActivityFrame;
public:
    ModuleActivity(ActivityFrame* parent, const std::string& title, Size size = Size(900, 650));
    virtual ~ModuleActivity();
    ModuleActivity(const ModuleActivity&) = delete;
    ModuleActivity& operator=(const ModuleActivity&) = delete;
    virtual void OnCreate() {}
    void Navigate(const std::string& name);
    bool GoBack();
    bool CanGoBack() const;
    void SetContent(const LayoutScope::Builder& build);
    void Show();
    void Close();
    bool IsOpen() const;
    void Message(const std::string& text, const std::string& title = "Message");
    bool Confirm(const std::string& text, const std::string& title = "Confirm");
    void PreviewText(const std::string& text, const std::string& title = "Preview");
};
}
