#include "internal.h"
namespace uiutils {
using namespace detail;
int LayoutState::Flags(Modifier m) const {
    int flags = m.margin ? wxALL : 0;
    if (m.expand) return flags | wxEXPAND;
    bool vertical = sizer->GetOrientation() == wxVERTICAL;
    if (m.alignment == Alignment::Center) flags |= vertical ? wxALIGN_CENTER_HORIZONTAL : wxALIGN_CENTER_VERTICAL;
    if (m.alignment == Alignment::End) flags |= vertical ? wxALIGN_RIGHT : wxALIGN_BOTTOM;
    return flags;
}
std::shared_ptr<ElementState> LayoutState::Add(wxWindow* w, Role role, Modifier m, WidgetStyle style) {
    sizer->Add(w, m.weight, Flags(m), parent->FromDIP(m.margin)); theme->Register(w, role, std::move(style));
    return std::make_shared<ElementState>(w, theme);
}
void LayoutScope::Box(bool vertical, const Builder& build, Modifier m) {
    auto* s = new wxBoxSizer(vertical ? wxVERTICAL : wxHORIZONTAL);
    state_->sizer->Add(s, m.weight, state_->Flags(m), state_->parent->FromDIP(m.margin));
    LayoutScope child(std::make_shared<LayoutState>(LayoutState{state_->parent, s, state_->theme})); build(child);
}
ScrollElement LayoutScope::Scroll(bool vertical, const Builder& build, Modifier m, bool bothAxes) {
    auto* w = new wxScrolledWindow(state_->parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, bothAxes ? (wxVSCROLL | wxHSCROLL) : (vertical ? wxVSCROLL : wxHSCROLL));
    w->SetMinSize(wxSize(1, 1)); int step = w->FromDIP(12); w->SetScrollRate((bothAxes || !vertical) ? step : 0, (bothAxes || vertical) ? step : 0);
    auto* s = new wxBoxSizer(vertical ? wxVERTICAL : wxHORIZONTAL); w->SetSizer(s);
    ScrollElement result(state_->Add(w, Role::Window, m, {}));
    LayoutScope child(std::make_shared<LayoutState>(LayoutState{w, s, state_->theme})); build(child);
    result.RefreshLayout(); return result;
}
void LayoutScope::Spacer(int dip) { if (dip < 0) throw std::invalid_argument("Negative spacer"); state_->sizer->AddSpacer(state_->parent->FromDIP(dip)); }
void LayoutScope::Stretch(int weight) { if (weight <= 0) throw std::invalid_argument("Stretch weight must be positive"); state_->sizer->AddStretchSpacer(weight); }
TextElement LayoutScope::Text(const std::string& value, Modifier m, WidgetStyle style) {
    return TextElement(state_->Add(new wxStaticText(state_->parent, wxID_ANY, W(value)), Role::Text, m, std::move(style)));
}
ButtonElement LayoutScope::Button(const std::string& label, std::function<void()> click, Modifier m, WidgetStyle style) {
    auto* w = new wxButton(state_->parent, wxID_ANY, W(label));
    if (click) w->Bind(wxEVT_BUTTON, [click = std::move(click)](wxCommandEvent&) { click(); });
    return ButtonElement(state_->Add(w, Role::Button, m, std::move(style)));
}
TextFieldElement LayoutScope::TextField(const std::string& value, Modifier m, WidgetStyle style,
    std::function<void(const std::string&)> changed, TextInputMode mode) {
    long flags = 0;
    switch(mode) {
    case TextInputMode::SingleLine: break;
    case TextInputMode::MultiLine: flags = wxTE_MULTILINE; break;
    case TextInputMode::Password: flags = wxTE_PASSWORD; break;
    case TextInputMode::ReadOnly: flags = wxTE_READONLY; break;
    }
    auto* w = new wxTextCtrl(state_->parent, wxID_ANY, W(value), wxDefaultPosition, wxDefaultSize, flags);
    if (changed) w->Bind(wxEVT_TEXT, [changed = std::move(changed)](wxCommandEvent& e) { changed(S(e.GetString())); });
    return TextFieldElement(state_->Add(w, Role::Input, m, std::move(style)));
}
CheckBoxElement LayoutScope::CheckBox(const std::string& label, bool checked, std::function<void(bool)> changed, Modifier m, WidgetStyle style) {
    auto* w = new wxCheckBox(state_->parent, wxID_ANY, W(label)); w->SetValue(checked);
    if (changed) w->Bind(wxEVT_CHECKBOX, [changed = std::move(changed)](wxCommandEvent& e) { changed(e.IsChecked()); });
    return CheckBoxElement(state_->Add(w, Role::Text, m, std::move(style)));
}
ToggleElement LayoutScope::ToggleButton(const std::string& label, bool checked, std::function<void(bool)> changed, Modifier m, WidgetStyle style) {
    auto* w = new wxToggleButton(state_->parent, wxID_ANY, W(label)); w->SetValue(checked);
    if (changed) w->Bind(wxEVT_TOGGLEBUTTON, [changed = std::move(changed)](wxCommandEvent& e) { changed(e.IsChecked()); });
    return ToggleElement(state_->Add(w, Role::Button, m, std::move(style)));
}
ChoiceElement LayoutScope::Choice(const Items& items, int selection, std::function<void(int)> changed, Modifier m, WidgetStyle style) {
    Selection(selection, items.size());
    auto* w = new wxChoice(state_->parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, WA(items)); w->SetSelection(selection);
    if (changed) w->Bind(wxEVT_CHOICE, [changed = std::move(changed)](wxCommandEvent& e) { changed(e.GetSelection()); });
    return ChoiceElement(state_->Add(w, Role::Input, m, std::move(style)));
}
ComboBoxElement LayoutScope::ComboBox(const Items& items, const std::string& value,
    std::function<void(const std::string&)> changed, Modifier m, WidgetStyle style) {
    auto* w = new wxComboBox(state_->parent, wxID_ANY, W(value), wxDefaultPosition, wxDefaultSize, WA(items), wxCB_DROPDOWN);
    if (changed) w->Bind(wxEVT_TEXT, [changed = std::move(changed)](wxCommandEvent& e) { changed(S(e.GetString())); });
    return ComboBoxElement(state_->Add(w, Role::Input, m, std::move(style)));
}
RadioElement LayoutScope::RadioBox(const std::string& label, const Items& items, int selection,
    std::function<void(int)> changed, Modifier m, WidgetStyle style) {
    Selection(selection, items.size(), false);
    auto* w = new wxRadioBox(state_->parent, wxID_ANY, W(label), wxDefaultPosition, wxDefaultSize, WA(items), 1, wxRA_SPECIFY_COLS);
    w->SetSelection(selection);
    if (changed) w->Bind(wxEVT_RADIOBOX, [changed = std::move(changed)](wxCommandEvent& e) { changed(e.GetSelection()); });
    return RadioElement(state_->Add(w, Role::Text, m, std::move(style)));
}
SliderElement LayoutScope::Slider(int value, int minimum, int maximum, std::function<void(int)> changed, Modifier m, WidgetStyle style) {
    Range(value, minimum, maximum); auto* w = new wxSlider(state_->parent, wxID_ANY, value, minimum, maximum);
    if (changed) w->Bind(wxEVT_SLIDER, [changed = std::move(changed)](wxCommandEvent& e) { changed(e.GetInt()); });
    return SliderElement(state_->Add(w, Role::Window, m, std::move(style)));
}
SpinElement LayoutScope::SpinCtrl(int value, int minimum, int maximum, std::function<void(int)> changed, Modifier m, WidgetStyle style) {
    Range(value, minimum, maximum);
    auto* w = new wxSpinCtrl(state_->parent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, minimum, maximum, value);
    if (changed) w->Bind(wxEVT_SPINCTRL, [changed = std::move(changed)](wxSpinEvent& e) { changed(e.GetValue()); });
    return SpinElement(state_->Add(w, Role::Input, m, std::move(style)));
}
ProgressElement LayoutScope::ProgressBar(int range, int value, Modifier m, WidgetStyle style) {
    Range(value, 0, range); auto* w = new wxGauge(state_->parent, wxID_ANY, range); w->SetValue(value);
    return ProgressElement(state_->Add(w, Role::Window, m, std::move(style)));
}
ListBoxElement LayoutScope::ListBox(const Items& items, int selection, std::function<void(int)> changed, Modifier m, WidgetStyle style) {
    Selection(selection, items.size()); auto* w = new wxListBox(state_->parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, WA(items), wxLB_SINGLE);
    w->SetSelection(selection);
    if (changed) w->Bind(wxEVT_LISTBOX, [changed = std::move(changed)](wxCommandEvent& e) { changed(e.GetSelection()); });
    return ListBoxElement(state_->Add(w, Role::Input, m, std::move(style)));
}
Element LayoutScope::Separator(Modifier m) {
    auto* w = new wxStaticLine(state_->parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
        state_->sizer->GetOrientation() == wxVERTICAL ? wxLI_HORIZONTAL : wxLI_VERTICAL);
    return Element(state_->Add(w, Role::Window, m, {}));
}

ImageElement LayoutScope::Image(const std::string& path, Size size, Modifier m, WidgetStyle style) {
    return Image(ImageSource::File(path), size, m, std::move(style));
}
ImageElement LayoutScope::Image(const ImageSource& source, Size size, Modifier m, WidgetStyle style) {
    if (size.width <= 0 || size.height <= 0 || size.width > 4096 || size.height > 4096)
        throw std::invalid_argument("Image dimensions must be 1..4096 DIP");
    auto* w = new wxStaticBitmap(state_->parent, wxID_ANY, wxBitmap(1, 1));
    ImageElement result(state_->Add(w, Role::Window, m, std::move(style)));
    result.state_->imageSize = size; result.SetMinSize(size); result.SetSource(source); return result;
}
PdfElement LayoutScope::PdfViewer(const std::string& path, Size size, Modifier m, WidgetStyle style) {
    auto image = Image(ImageSource::File({}), size, m, std::move(style));
    PdfElement result(image.state_); result.state_->pdfBaseSize = size;
    if (!path.empty()) result.Open(path);
    return result;
}
}

namespace uiutils {
TabsElement LayoutScope::Tabs(const std::vector<TabPage>& pages, int selection,
    std::function<void(int)> changed, Modifier m, WidgetStyle style) {
    detail::Selection(selection, pages.size(), false);
    for (const auto& page : pages) if (!page.build) throw std::invalid_argument("Tab page requires a builder");
    auto* notebook = new wxNotebook(state_->parent, wxID_ANY);
    TabsElement result(state_->Add(notebook, detail::Role::Window, m, std::move(style)));
    try {
        for (const auto& page : pages) {
            auto* panel = new wxPanel(notebook);
            auto* sizer = new wxBoxSizer(wxVERTICAL); panel->SetSizer(sizer);
            notebook->AddPage(panel, detail::W(page.title));
            state_->theme->Register(panel, detail::Role::Window, {});
            LayoutScope child(std::make_shared<detail::LayoutState>(detail::LayoutState{panel, sizer, state_->theme}));
            page.build(child);
        }
        notebook->ChangeSelection(selection);
        if (changed) notebook->Bind(wxEVT_NOTEBOOK_PAGE_CHANGED,
            [changed = std::move(changed)](wxBookCtrlEvent& event) { changed(event.GetSelection()); event.Skip(); });
    } catch (...) { state_->sizer->Detach(notebook); notebook->Destroy(); throw; }
    return result;
}
}
