#pragma once
#include <uiutils/Elements.h>
#include <functional>
#include <utility>
namespace uiutils {
namespace detail { struct LayoutState; }
class ModuleActivity;
class LayoutScope {
    std::shared_ptr<detail::LayoutState> state_;
    explicit LayoutScope(std::shared_ptr<detail::LayoutState> state) : state_(std::move(state)) {}
    friend class ModuleActivity;
    void Box(bool vertical, const std::function<void(LayoutScope&)>& build, Modifier modifier);
    ScrollElement Scroll(bool vertical, const std::function<void(LayoutScope&)>& build, Modifier modifier, bool bothAxes = false);
public:
    using Builder = std::function<void(LayoutScope&)>;
    void Row(const Builder& build, Modifier modifier = {}) { Box(false, build, modifier); }
    void Column(const Builder& build, Modifier modifier = {}) { Box(true, build, modifier); }
    ScrollElement ScrollColumn(const Builder& build, Modifier m = {}) { return Scroll(true, build, m); }
    ScrollElement ScrollRow(const Builder& build, Modifier m = {}) { return Scroll(false, build, m); }
    ScrollElement ScrollArea(const Builder& build, Modifier m = {}) { return Scroll(true, build, m, true); }
    template<class F, class... Args> decltype(auto) Component(F&& function, Args&&... args) {
        return std::invoke(std::forward<F>(function), *this, std::forward<Args>(args)...);
    }
    struct TabPage { std::string title; Builder build; };
    TabsElement Tabs(const std::vector<TabPage>& pages, int selection = 0,
        std::function<void(int)> changed = {}, Modifier m = {}, WidgetStyle style = {});
    void Spacer(int dip);
    void Stretch(int weight = 1);
    TextElement Text(const std::string& value, Modifier m = {}, WidgetStyle style = {});
    ButtonElement Button(const std::string& label, std::function<void()> click = {}, Modifier m = {}, WidgetStyle style = {});
    TextFieldElement TextField(const std::string& value = {}, Modifier m = {}, WidgetStyle style = {},
        std::function<void(const std::string&)> changed = {}, TextInputMode mode = TextInputMode::SingleLine);
    CheckBoxElement CheckBox(const std::string& label, bool checked = false,
        std::function<void(bool)> changed = {}, Modifier m = {}, WidgetStyle style = {});
    ToggleElement ToggleButton(const std::string& label, bool checked = false,
        std::function<void(bool)> changed = {}, Modifier m = {}, WidgetStyle style = {});
    ChoiceElement Choice(const Items& items, int selection = NoSelection,
        std::function<void(int)> changed = {}, Modifier m = {}, WidgetStyle style = {});
    ComboBoxElement ComboBox(const Items& items, const std::string& value = {},
        std::function<void(const std::string&)> changed = {}, Modifier m = {}, WidgetStyle style = {});
    RadioElement RadioBox(const std::string& label, const Items& items, int selection = 0,
        std::function<void(int)> changed = {}, Modifier m = {}, WidgetStyle style = {});
    SliderElement Slider(int value, int minimum, int maximum, std::function<void(int)> changed = {}, Modifier m = {}, WidgetStyle style = {});
    SpinElement SpinCtrl(int value, int minimum, int maximum, std::function<void(int)> changed = {}, Modifier m = {}, WidgetStyle style = {});
    ProgressElement ProgressBar(int range = 100, int value = 0, Modifier m = {}, WidgetStyle style = {});
    ListBoxElement ListBox(const Items& items, int selection = NoSelection,
        std::function<void(int)> changed = {}, Modifier m = {}, WidgetStyle style = {});
    Element Separator(Modifier m = {});
    ImageElement Image(const ImageSource& source, Size size = Size(96, 96), Modifier m = {}, WidgetStyle style = {});
    PdfElement PdfViewer(const std::string& path = {}, Size size = Size(612, 792), Modifier m = {}, WidgetStyle style = {});
    ImageElement Image(const std::string& path = {}, Size size = Size(96, 96), Modifier m = {}, WidgetStyle style = {});
};
}
