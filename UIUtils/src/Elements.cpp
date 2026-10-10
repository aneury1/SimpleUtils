#include <wx/artprov.h>
#include <wx/image.h>
#include <wx/log.h>
#include "internal.h"
namespace uiutils {
using namespace detail;
template <class T>
T* Native(const Element& e) {
  return static_cast<T*>(ElementAccess::Native(e));
}
static void Relayout(wxWindow* w) {
  w->InvalidateBestSize();
  for (wxWindow* p = w; p; p = p->GetParent()) {
    p->Layout();
    if (auto* scroll = dynamic_cast<wxScrolledWindow*>(p))
      scroll->FitInside();
  }
}
bool Element::IsValid() const {
  return state_ && state_->window;
}
void Element::Enable(bool v) const {
  WidgetStyle s;
  s.Set<StyleKey::Enabled>(v);
  ApplyStyle(s);
}
bool Element::IsEnabled() const {
  return ElementAccess::Native(*this)->IsEnabled();
}
void Element::Show(bool v) const {
  WidgetStyle s;
  s.Set<StyleKey::Visible>(v);
  ApplyStyle(s);
  Relayout(ElementAccess::Native(*this));
}
void Element::Focus() const {
  ElementAccess::Native(*this)->SetFocus();
}
void Element::SetMinSize(Size v) const {
  WidgetStyle s;
  s.Set<StyleKey::MinSize>(v);
  ApplyStyle(s);
  Relayout(ElementAccess::Native(*this));
}
void Element::SetTooltip(const std::string& v) const {
  WidgetStyle s;
  s.Set<StyleKey::Tooltip>(v);
  ApplyStyle(s);
}
void Element::ApplyStyle(const WidgetStyle& s) const {
  auto* w = ElementAccess::Native(*this);
  state_->theme->Override(w, s);
}
void TextElement::SetText(const std::string& s) const {
  auto* w = Native<wxStaticText>(*this);
  w->SetLabel(W(s));
  Relayout(w);
}
std::string TextElement::GetText() const {
  return S(Native<wxStaticText>(*this)->GetLabel());
}
void ButtonElement::SetText(const std::string& s) const {
  auto* w = Native<wxButton>(*this);
  w->SetLabel(W(s));
  Relayout(w);
}
std::string TextFieldElement::GetValue() const {
  return S(Native<wxTextCtrl>(*this)->GetValue());
}
void TextFieldElement::SetValue(const std::string& s, bool notify) const {
  auto* w = Native<wxTextCtrl>(*this);
  if (notify)
    w->SetValue(W(s));
  else
    w->ChangeValue(W(s));
}
void TextFieldElement::SetEditable(bool v) const {
  Native<wxTextCtrl>(*this)->SetEditable(v);
}
bool CheckBoxElement::GetValue() const {
  return Native<wxCheckBox>(*this)->GetValue();
}
void CheckBoxElement::SetValue(bool v) const {
  Native<wxCheckBox>(*this)->SetValue(v);
}
bool ToggleElement::GetValue() const {
  return Native<wxToggleButton>(*this)->GetValue();
}
void ToggleElement::SetValue(bool v) const {
  Native<wxToggleButton>(*this)->SetValue(v);
}
int ChoiceElement::GetSelection() const {
  return Native<wxChoice>(*this)->GetSelection();
}
void ChoiceElement::SetSelection(int i) const {
  auto* w = Native<wxChoice>(*this);
  Selection(i, w->GetCount());
  w->SetSelection(i);
}
std::string ComboBoxElement::GetValue() const {
  return S(Native<wxComboBox>(*this)->GetValue());
}
void ComboBoxElement::SetValue(const std::string& s, bool notify) const {
  auto* w = Native<wxComboBox>(*this);
  if (notify)
    w->SetValue(W(s));
  else
    w->ChangeValue(W(s));
}
int RadioElement::GetSelection() const {
  return Native<wxRadioBox>(*this)->GetSelection();
}
void RadioElement::SetSelection(int i) const {
  auto* w = Native<wxRadioBox>(*this);
  Selection(i, w->GetCount(), false);
  w->SetSelection(i);
}
int SliderElement::GetValue() const {
  return Native<wxSlider>(*this)->GetValue();
}
void SliderElement::SetValue(int v) const {
  auto* w = Native<wxSlider>(*this);
  Range(v, w->GetMin(), w->GetMax());
  w->SetValue(v);
}
int SpinElement::GetValue() const {
  return Native<wxSpinCtrl>(*this)->GetValue();
}
void SpinElement::SetValue(int v) const {
  auto* w = Native<wxSpinCtrl>(*this);
  Range(v, w->GetMin(), w->GetMax());
  w->SetValue(v);
}
int ProgressElement::GetValue() const {
  return Native<wxGauge>(*this)->GetValue();
}
void ProgressElement::SetValue(int v) const {
  auto* w = Native<wxGauge>(*this);
  Range(v, 0, w->GetRange());
  w->SetValue(v);
}
void ProgressElement::Pulse() const {
  Native<wxGauge>(*this)->Pulse();
}
void ListBoxElement::SetItems(const Items& items) const {
  Native<wxListBox>(*this)->Set(WA(items));
}
int ListBoxElement::GetSelection() const {
  return Native<wxListBox>(*this)->GetSelection();
}
void ListBoxElement::SetSelection(int i) const {
  auto* w = Native<wxListBox>(*this);
  Selection(i, w->GetCount());
  w->SetSelection(i);
}
int ListBoxElement::Count() const {
  return static_cast<int>(Native<wxListBox>(*this)->GetCount());
}
void ScrollElement::RefreshLayout() const {
  auto* w = Native<wxScrolledWindow>(*this);
  w->Layout();
  w->FitInside();
}

bool ImageElement::SetSource(const std::string& path) const {
  return SetSource(ImageSource::File(path));
}
bool ImageElement::SetSource(const ImageSource& source) const {
  auto* w = Native<wxStaticBitmap>(*this);
  wxSize target = w->FromDIP(WS(state_->imageSize));
  wxBitmap bitmap;
  bool loaded = RenderSource(*state_, source, Size(target.x, target.y), bitmap);
  if (!loaded)
    bitmap = wxArtProvider::GetBitmap(wxART_MISSING_IMAGE, wxART_OTHER,
                                      wxSize(32, 32));
  if (bitmap.IsOk())
    w->SetBitmap(bitmap);
  Relayout(w);
  return loaded;
}
std::string ImageElement::LastError() const {
  ElementAccess::Native(*this);
  return state_->error;
}
bool ImageElement::SavePng(const std::string& path) const {
  auto* w = Native<wxStaticBitmap>(*this);
  if (!state_->error.empty())
    return false;
  return w->GetBitmap().ConvertToImage().SaveFile(W(path), wxBITMAP_TYPE_PNG);
}
bool PdfElement::Open(const std::string& path) const {
  ElementAccess::Native(*this);
  state_->pdf.reset();
  state_->pdfPath = path;
  state_->pdfPage = 0;
  return ImageElement(state_).SetSource(ImageSource::Pdf(path));
}
int PdfElement::PageCount() const {
  ElementAccess::Native(*this);
  return PdfPageCount(state_->pdf);
}
int PdfElement::CurrentPage() const {
  ElementAccess::Native(*this);
  return PageCount() ? state_->pdfPage : NoSelection;
}
bool PdfElement::GoToPage(int index) const {
  ElementAccess::Native(*this);
  if (index < 0 || index >= PageCount()) {
    state_->error = "PDF page is out of range";
    return false;
  }
  if (!ImageElement(state_).SetSource(ImageSource::Pdf(state_->pdfPath, index)))
    return false;
  state_->pdfPage = index;
  return true;
}
bool PdfElement::SetZoom(double zoom) const {
  ElementAccess::Native(*this);
  if (!std::isfinite(zoom) || zoom < 0.25 || zoom > 3.0) {
    state_->error = "Zoom must be 0.25..3.0";
    return false;
  }
  if (!PageCount()) {
    state_->error = "Open a PDF first";
    return false;
  }
  Size previous = state_->imageSize;
  state_->imageSize = Size(int(state_->pdfBaseSize.width * zoom),
                           int(state_->pdfBaseSize.height * zoom));
  if (!ImageElement(state_).SetSource(
          ImageSource::Pdf(state_->pdfPath, state_->pdfPage))) {
    state_->imageSize = previous;
    return false;
  }
  state_->pdfZoom = zoom;
  SetMinSize(state_->imageSize);
  return true;
}
double PdfElement::GetZoom() const {
  ElementAccess::Native(*this);
  return state_->pdfZoom;
}
std::string PdfElement::LastError() const {
  ElementAccess::Native(*this);
  return state_->error;
}
}  // namespace uiutils
