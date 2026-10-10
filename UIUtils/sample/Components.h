#pragma once
#include <uiutils/Controls.h>
#include "RestaurantModel.h"
namespace components {
inline uiutils::TextElement Heading(uiutils::LayoutScope& ui,
                                    const std::string& title) {
  uiutils::WidgetStyle style;
  style.Set<uiutils::StyleKey::FontSize>(16).Set<uiutils::StyleKey::Bold>(true);
  return ui.Text(title, uiutils::Modifier{}.Margin(8), style);
}
inline uiutils::ButtonElement ProductCard(uiutils::LayoutScope& ui,
                                          const restaurant::Product& product,
                                          std::function<void()> onClick) {
  uiutils::WidgetStyle style;
  style.Set<uiutils::StyleKey::MinSize>(uiutils::Size(150, 64));
  return ui.Button(product.name + "\n" + restaurant::Money(product.cents),
                   std::move(onClick),
                   uiutils::Modifier{}.Weight().Expand().Margin(6), style);
}
}  // namespace components
