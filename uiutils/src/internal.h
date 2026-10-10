#pragma once
#include <uiutils/UiApplication.h>
#include <wx/wx.h>
#include <wx/settings.h>
#include <wx/mdi.h>
#include <wx/weakref.h>
#include <wx/scrolwin.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/combobox.h>
#include <wx/gauge.h>
#include <wx/listbox.h>
#include <wx/radiobox.h>
#include <wx/slider.h>
#include <wx/spinctrl.h>
#include <wx/statline.h>
#include <wx/statbmp.h>
#include <wx/tglbtn.h>
#include <algorithm>
#include <map>
#include <wx/notebook.h>
namespace uiutils {
namespace detail {
inline wxString W(const std::string& s) { return wxString::FromUTF8(s.c_str()); }
inline std::string S(const wxString& s) { return s.ToStdString(wxConvUTF8); }
inline wxSize WS(Size s) { return wxSize(s.width, s.height); }
inline wxArrayString WA(const Items& items) { wxArrayString a; for (const auto& s : items) a.Add(W(s)); return a; }
enum class Role { Window, Text, Button, Input };
void ApplyStyle(wxWindow& window, const WidgetStyle& style);
struct ThemeContext {
    struct Entry { wxWeakRef<wxWindow> window; Role role; WidgetStyle overrides; };
    Theme theme = LightTheme();
    std::vector<Entry> entries;
    void Apply(Entry& entry);
    void Register(wxWindow* w, Role role, WidgetStyle overrides);
    void Override(wxWindow* w, const WidgetStyle& style);
    void SetTheme(Theme next);
};
struct PdfState;
struct ElementState {
    wxWeakRef<wxWindow> window;
    std::shared_ptr<ThemeContext> theme;
    Size imageSize;
    Size pdfBaseSize;
    std::shared_ptr<PdfState> pdf;
    std::string pdfPath;
    int pdfPage = 0;
    double pdfZoom = 1.0;
    std::string error;
    ElementState(wxWindow* w, std::shared_ptr<ThemeContext> t) : window(w), theme(std::move(t)) {}
};
bool RenderSource(ElementState& state, const ImageSource& source, Size pixels, wxBitmap& bitmap);
bool RenderCodes(const ImageSource& source, Size pixels, wxBitmap& bitmap, std::string& error);
bool RenderChart(const ChartData& data, Size pixels, wxBitmap& bitmap, std::string& error);
bool RenderPdf(ElementState& state, const PdfPageImage& source, Size pixels, wxBitmap& bitmap);
int PdfPageCount(const std::shared_ptr<PdfState>& state);
struct ElementAccess {
    static wxWindow* Native(const Element& e) {
        if (!e.state_ || !e.state_->window) throw std::logic_error("Element is empty or its window was destroyed");
        return e.state_->window.get();
    }
    static const std::shared_ptr<ElementState>& State(const Element& e) { return e.state_; }
};
struct LayoutState {
    wxWindow* parent;
    wxBoxSizer* sizer;
    std::shared_ptr<ThemeContext> theme;
    int Flags(Modifier m) const;
    std::shared_ptr<ElementState> Add(wxWindow* window, Role role, Modifier m, WidgetStyle style);
};
struct ActivityState {
    wxWeakRef<wxMDIParentFrame> window;
    std::shared_ptr<ThemeContext> theme = std::make_shared<ThemeContext>();
    std::vector<std::unique_ptr<ModuleActivity>> modules;
    std::map<std::string, ActivityFrame::FrameFactory> factories;
    std::vector<std::string> history;
    bool navigating = false;
};
struct ModuleState {
    wxWeakRef<wxMDIChildFrame> window;
    std::shared_ptr<ThemeContext> theme;
    bool content = false;
    bool closing = false;
    ActivityFrame* parent = nullptr;
    std::string name;
};
inline void Selection(int index, size_t count, bool allowNone = true) {
    if (allowNone && index == NoSelection) return;
    if (index < 0 || static_cast<size_t>(index) >= count) throw std::out_of_range("Invalid selection");
}
inline void Range(int value, int minimum, int maximum) {
    if (minimum >= maximum || value < minimum || value > maximum) throw std::out_of_range("Invalid range or value");
}
}
// Internal application class; public headers contain no wxWidgets types.
class UIApplication final : public wxApp {
    ActivityFactory factory_;
    std::unique_ptr<ActivityFrame> activity_;
public:
    explicit UIApplication(ActivityFactory factory) : factory_(std::move(factory)) {}
    bool OnInit() override;
    int OnExit() override;
};
}
