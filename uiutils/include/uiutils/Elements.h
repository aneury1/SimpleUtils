#pragma once
#include <uiutils/Style.h>
#include <uiutils/ImageSource.h>
#include <memory>
namespace uiutils {
namespace detail { struct ElementState; struct ElementAccess; }
class LayoutScope;
class Element {
protected:
    std::shared_ptr<detail::ElementState> state_;
    explicit Element(std::shared_ptr<detail::ElementState> state) : state_(std::move(state)) {}
    friend class LayoutScope;
    friend struct detail::ElementAccess;
public:
    Element() = default;
    bool IsValid() const;
    void Enable(bool enabled = true) const;
    bool IsEnabled() const;
    void Show(bool visible = true) const;
    void Focus() const;
    void SetMinSize(Size size) const;
    void SetTooltip(const std::string& text) const;
    void ApplyStyle(const WidgetStyle& style) const;
};
#define UIUTILS_ELEMENT_CTOR(Type) \
    friend class LayoutScope; \
    explicit Type(std::shared_ptr<detail::ElementState> state) : Element(std::move(state)) {} \
public: Type() = default;
class TextElement : public Element {
    UIUTILS_ELEMENT_CTOR(TextElement)
    void SetText(const std::string& text) const;
    std::string GetText() const;
};
class ButtonElement : public Element {
    UIUTILS_ELEMENT_CTOR(ButtonElement)
    void SetText(const std::string& text) const;
};
class TextFieldElement : public Element {
    UIUTILS_ELEMENT_CTOR(TextFieldElement)
    std::string GetValue() const;
    void SetValue(const std::string& text, bool notify = false) const;
    void SetEditable(bool editable) const;
};
class CheckBoxElement : public Element {
    UIUTILS_ELEMENT_CTOR(CheckBoxElement)
    bool GetValue() const;
    void SetValue(bool value) const;
};
class ToggleElement : public Element {
    UIUTILS_ELEMENT_CTOR(ToggleElement)
    bool GetValue() const;
    void SetValue(bool value) const;
};
class ChoiceElement : public Element {
    UIUTILS_ELEMENT_CTOR(ChoiceElement)
    int GetSelection() const;
    void SetSelection(int index) const;
};
class ComboBoxElement : public Element {
    UIUTILS_ELEMENT_CTOR(ComboBoxElement)
    std::string GetValue() const;
    void SetValue(const std::string& value, bool notify = false) const;
};
class RadioElement : public Element {
    UIUTILS_ELEMENT_CTOR(RadioElement)
    int GetSelection() const;
    void SetSelection(int index) const;
};
class SliderElement : public Element {
    UIUTILS_ELEMENT_CTOR(SliderElement)
    int GetValue() const;
    void SetValue(int value) const;
};
class SpinElement : public Element {
    UIUTILS_ELEMENT_CTOR(SpinElement)
    int GetValue() const;
    void SetValue(int value) const;
};
class ProgressElement : public Element {
    UIUTILS_ELEMENT_CTOR(ProgressElement)
    int GetValue() const;
    void SetValue(int value) const;
    void Pulse() const;
};
class ListBoxElement : public Element {
    UIUTILS_ELEMENT_CTOR(ListBoxElement)
    void SetItems(const Items& items) const;
    int GetSelection() const;
    void SetSelection(int index) const;
    int Count() const;
};
class ImageElement : public Element {
    UIUTILS_ELEMENT_CTOR(ImageElement)
    // Failure displays a built-in placeholder and returns false.
    bool SetSource(const std::string& path) const;
    bool SetSource(const ImageSource& source) const;
    std::string LastError() const;
    bool SavePng(const std::string& path) const;
    friend class PdfElement;
};
class PdfElement : public Element {
    UIUTILS_ELEMENT_CTOR(PdfElement)
    bool Open(const std::string& path) const;
    int PageCount() const;
    int CurrentPage() const;
    bool GoToPage(int index) const;
    bool SetZoom(double zoom) const;
    double GetZoom() const;
    std::string LastError() const;
};
class TabsElement : public Element {
    UIUTILS_ELEMENT_CTOR(TabsElement)
    int Count() const;
    int GetSelection() const;
    void SetSelection(int index, bool notify = false) const;
    std::string GetPageTitle(int index) const;
    void SetPageTitle(int index, const std::string& title) const;
    void RemovePage(int index) const;
};
class ScrollElement : public Element {
    UIUTILS_ELEMENT_CTOR(ScrollElement)
    void RefreshLayout() const;
};
#undef UIUTILS_ELEMENT_CTOR
}
