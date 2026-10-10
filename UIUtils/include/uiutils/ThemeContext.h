#pragma once
#include <uiutils/DefaultThemes.h>
#include <wx/weakref.h>
#include <algorithm>
#include <vector>
namespace uiutils {
enum class ThemeRole { Window, Text, Button, Input };
class ThemeContext {
  struct Entry {
    wxWeakRef<wxWindow> window;
    ThemeRole role;
    WidgetStyle overrides;
  };
  Theme theme_ = LightTheme();
  std::vector<Entry> entries_;
  void Apply(Entry& e) {
    if (!e.window)
      return;
    WidgetStyle style;
    switch (e.role) {
      case ThemeRole::Window:
        style = theme_.window;
        break;
      case ThemeRole::Text:
        style = theme_.text;
        break;
      case ThemeRole::Button:
        style = theme_.button;
        break;
      case ThemeRole::Input:
        style = theme_.input;
        break;
    }
    style.Merge(e.overrides).Apply(*e.window);
  }

 public:
  void Register(wxWindow* window, ThemeRole role, WidgetStyle overrides = {}) {
    entries_.push_back(
        {wxWeakRef<wxWindow>(window), role, std::move(overrides)});
    Apply(entries_.back());
  }
  void SetTheme(Theme theme) {
    theme_ = std::move(theme);
    entries_.erase(std::remove_if(entries_.begin(), entries_.end(),
                                  [](const Entry& e) { return !e.window; }),
                   entries_.end());
    for (auto& e : entries_)
      Apply(e);
  }
};
}  // namespace uiutils
