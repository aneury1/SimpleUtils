#include <gtest/gtest.h>
#include <uiutils/DefaultThemes.h>
using namespace uiutils;
TEST(Styles, RejectsInvalidColorAndRuntimeValues) {
    EXPECT_THROW(Color("red"), std::invalid_argument);
    EXPECT_THROW(Color("#ZZZZZZ"), std::invalid_argument);
    EXPECT_THROW(WidgetStyle{}.Set(StyleKey::FontSize, 0), std::invalid_argument);
    EXPECT_THROW(WidgetStyle{}.Set(StyleKey::Bold, 12), std::invalid_argument);
    EXPECT_THROW(WidgetStyle{}.Set(StyleKey::MinSize, Size(-2, 5)), std::invalid_argument);
}
TEST(Styles, OverridesWinWithoutChangingDefaults) {
    auto base = LightTheme().text;
    WidgetStyle overrides;
    overrides.Set<StyleKey::FontSize>(18).Set<StyleKey::Bold>(true);
    auto merged = base; merged.Merge(overrides);
    ASSERT_NE(merged.Get<int>(StyleKey::FontSize), nullptr);
    EXPECT_EQ(*merged.Get<int>(StyleKey::FontSize), 18);
    EXPECT_EQ(*base.Get<int>(StyleKey::FontSize), 11);
    EXPECT_EQ(merged.Get<Color>(StyleKey::Foreground)->hex, base.Get<Color>(StyleKey::Foreground)->hex);
    merged.Remove(StyleKey::Bold); EXPECT_FALSE(merged.Contains(StyleKey::Bold));
}
TEST(Styles, ThemesHaveDifferentBackgrounds) {
    EXPECT_NE(LightTheme().window.Get<Color>(StyleKey::Background)->hex,
              DarkTheme().window.Get<Color>(StyleKey::Background)->hex);
}
TEST(Layout, ModifiersAreImmutableAndRejectInvalidValues) {
    Modifier base;
    auto item = base.Weight(2).Expand().Margin(8).Align(Alignment::Center);
    EXPECT_EQ(base.weight, 0); EXPECT_FALSE(base.expand);
    EXPECT_EQ(item.weight, 2); EXPECT_TRUE(item.expand); EXPECT_EQ(item.margin, 8);
    EXPECT_THROW(base.Weight(-1), std::invalid_argument);
    EXPECT_THROW(base.Margin(-1), std::invalid_argument);
}
