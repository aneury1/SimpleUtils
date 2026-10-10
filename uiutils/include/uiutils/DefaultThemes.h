#pragma once

#include <uiutils/Style.h>

namespace uiutils {

struct Theme {
    WidgetStyle window;
    WidgetStyle text;
    WidgetStyle button;
    WidgetStyle input;
};

inline Theme LightTheme() {
    Theme theme;

    theme.window
        .Set(StyleKey::Background, Color("#F5F7FA"))
        .Set(StyleKey::Foreground, Color("#182230"))
        .Set(StyleKey::FontSize, 11);

    theme.text
        .Set(StyleKey::Background, Color("#F5F7FA"))
        .Set(StyleKey::Foreground, Color("#182230"))
        .Set(StyleKey::FontSize, 11);

    theme.button
        .Set(StyleKey::Background, Color("#1565C0"))
        .Set(StyleKey::Foreground, Color("#FFFFFF"))
        .Set(StyleKey::Bold, true)
        .Set(StyleKey::MinSize, Size(140, 40));

    theme.input
        .Set(StyleKey::Background, Color("#FFFFFF"))
        .Set(StyleKey::Foreground, Color("#182230"))
        .Set(StyleKey::FontSize, 11);

    return theme;
}

inline Theme DarkTheme() {
    Theme theme;

    theme.window
        .Set(StyleKey::Background, Color("#121820"))
        .Set(StyleKey::Foreground, Color("#E6EDF3"))
        .Set(StyleKey::FontSize, 11);

    theme.text
        .Set(StyleKey::Background, Color("#121820"))
        .Set(StyleKey::Foreground, Color("#E6EDF3"))
        .Set(StyleKey::FontSize, 11);

    theme.button
        .Set(StyleKey::Background, Color("#90CAF9"))
        .Set(StyleKey::Foreground, Color("#102030"))
        .Set(StyleKey::Bold, true)
        .Set(StyleKey::MinSize, Size(140, 40));

    theme.input
        .Set(StyleKey::Background, Color("#202B38"))
        .Set(StyleKey::Foreground, Color("#E6EDF3"))
        .Set(StyleKey::FontSize, 11);

    return theme;
}

} // namespace uiutils
