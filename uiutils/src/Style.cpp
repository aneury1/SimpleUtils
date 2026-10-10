#include "internal.h"
namespace uiutils {
void WidgetStyle::Apply(const Element& element) const { element.ApplyStyle(*this); }
namespace detail {
void ApplyStyle(wxWindow& w, const WidgetStyle& s) {
    if (auto* v = s.Get<Color>(StyleKey::Background)) w.SetBackgroundColour(wxColour(W(v->hex)));
    if (auto* v = s.Get<Color>(StyleKey::Foreground)) w.SetForegroundColour(wxColour(W(v->hex)));
    auto font = w.GetFont(); if (!font.IsOk()) font = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
    bool changed = false;
    if (auto* v = s.Get<std::string>(StyleKey::FontFace)) { font.SetFaceName(W(*v)); changed = true; }
    if (auto* v = s.Get<int>(StyleKey::FontSize)) { font.SetPointSize(*v); changed = true; }
    if (auto* v = s.Get<bool>(StyleKey::Bold)) { font.SetWeight(*v ? wxFONTWEIGHT_BOLD : wxFONTWEIGHT_NORMAL); changed = true; }
    if (auto* v = s.Get<bool>(StyleKey::Italic)) { font.SetStyle(*v ? wxFONTSTYLE_ITALIC : wxFONTSTYLE_NORMAL); changed = true; }
    if (auto* v = s.Get<bool>(StyleKey::Underline)) { font.SetUnderlined(*v); changed = true; }
    if (changed) w.SetFont(font);
    if (auto* v = s.Get<Size>(StyleKey::MinSize)) w.SetMinSize(w.FromDIP(WS(*v)));
    if (auto* v = s.Get<Size>(StyleKey::MaxSize)) w.SetMaxSize(w.FromDIP(WS(*v)));
    if (auto* v = s.Get<std::string>(StyleKey::Tooltip)) w.SetToolTip(W(*v));
    if (auto* v = s.Get<bool>(StyleKey::Enabled)) w.Enable(*v);
    if (auto* v = s.Get<bool>(StyleKey::Visible)) w.Show(*v);
    w.InvalidateBestSize(); w.Layout(); if (auto* parent = w.GetParent()) parent->Layout(); w.Refresh();
}
void ThemeContext::Apply(Entry& e) {
    if (!e.window) return;
    WidgetStyle style;
    switch(e.role) {
    case Role::Window: style = theme.window; break;
    case Role::Text: style = theme.text; break;
    case Role::Button: style = theme.button; break;
    case Role::Input: style = theme.input; break;
    }
    ApplyStyle(*e.window, style.Merge(e.overrides));
}
void ThemeContext::Register(wxWindow* w, Role role, WidgetStyle style) {
    entries.push_back({wxWeakRef<wxWindow>(w), role, std::move(style)}); Apply(entries.back());
}
void ThemeContext::Override(wxWindow* w, const WidgetStyle& style) {
    for (auto& e : entries) if (e.window.get() == w) { e.overrides.Merge(style); Apply(e); return; }
}
void ThemeContext::SetTheme(Theme next) {
    theme = std::move(next);
    entries.erase(std::remove_if(entries.begin(), entries.end(), [](const Entry& e) { return !e.window; }), entries.end());
    for (auto& e : entries) Apply(e);
}
}
}
