#include <wx/dialog.h>
#include "internal.h"
namespace uiutils {
using namespace detail;
ActivityFrame::ActivityFrame(const std::string& title, Size size)
    : state_(std::make_unique<ActivityState>()) {
  auto* w = new wxMDIParentFrame(nullptr, wxID_ANY, W(title), wxDefaultPosition,
                                 wxDefaultSize);
  w->SetSize(w->FromDIP(WS(size)));
  state_->window = w;
  w->SetMenuBar(new wxMenuBar());
  state_->theme->Register(w, Role::Window, {});
  if (auto* client = w->GetClientWindow())
    state_->theme->Register(client, Role::Window, {});
}
ActivityFrame::~ActivityFrame() {
  state_->modules.clear();
  if (state_->window)
    state_->window->Destroy();
}
void ActivityFrame::AddModule(std::unique_ptr<ModuleActivity> module) {
  auto& modules = state_->modules;
  modules.erase(std::remove_if(modules.begin(), modules.end(),
                               [](const auto& m) { return !m->IsOpen(); }),
                modules.end());
  modules.push_back(std::move(module));
}
void ActivityFrame::SetTheme(Theme theme) {
  state_->theme->SetTheme(std::move(theme));
}
int ActivityFrame::addMenuBarOption(const std::string& label, int action) {
  return addMenuBarOption(label, label, action);
}
int ActivityFrame::addMenuBarOption(const std::string& label,
                                    const std::string& item, int action) {
  if (!state_->window)
    throw std::logic_error("Activity is closed");
  auto* bar = state_->window->GetMenuBar();
  int index = bar->FindMenu(W(label));
  wxMenu* menu;
  if (index == wxNOT_FOUND) {
    menu = new wxMenu();
    bar->Append(menu, W(label));
  } else
    menu = bar->GetMenu(index);
  return menu->Append(action, W(item))->GetId();
}
void ActivityFrame::OnMenu(int id, std::function<void()> callback) {
  state_->window->Bind(
      wxEVT_MENU,
      [callback = std::move(callback)](wxCommandEvent&) {
        if (callback)
          callback();
      },
      id);
}
void ActivityFrame::AddExitMenu(const std::string& label) {
  int id = addMenuBarOption(label, "E&xit");
  OnMenu(id, [this] { Close(); });
}
void ActivityFrame::Show() {
  if (state_->window)
    state_->window->Show();
}
void ActivityFrame::Close() {
  if (state_->window)
    state_->window->Close();
}
ModuleActivity::ModuleActivity(ActivityFrame* parent, const std::string& title,
                               Size size)
    : state_(std::make_unique<ModuleState>()) {
  if (!parent || !parent->state_->window)
    throw std::invalid_argument("Module requires an open parent");
  auto* w = new wxMDIChildFrame(parent->state_->window.get(), wxID_ANY,
                                W(title), wxDefaultPosition, wxDefaultSize);
  w->SetSize(w->FromDIP(WS(size)));
  state_->window = w;
  state_->theme = parent->state_->theme;
  state_->theme->Register(w, Role::Window, {});
}
ModuleActivity::~ModuleActivity() {
  if (state_->window)
    state_->window->Destroy();
}
bool ModuleActivity::IsOpen() const {
  return state_->window && !state_->window->IsBeingDeleted();
}
void ModuleActivity::Show() {
  if (IsOpen())
    state_->window->Show();
}
void ModuleActivity::Close() {
  if (IsOpen())
    state_->window->Close();
}
void ModuleActivity::SetContent(const LayoutScope::Builder& build) {
  if (!IsOpen())
    throw std::logic_error("Module is closed");
  if (state_->content)
    throw std::logic_error("SetContent can only be called once");
  state_->content = true;
  auto* w = state_->window.get();
  auto* panel = new wxPanel(w);
  auto* root = new wxBoxSizer(wxVERTICAL);
  root->Add(panel, 1, wxEXPAND);
  w->SetSizer(root);
  auto* content = new wxBoxSizer(wxVERTICAL);
  panel->SetSizer(content);
  state_->theme->Register(panel, Role::Window, {});
  LayoutScope scope(std::make_shared<LayoutState>(
      LayoutState{panel, content, state_->theme}));
  build(scope);
  w->Layout();
}
void ModuleActivity::Message(const std::string& text,
                             const std::string& title) {
  wxMessageBox(W(text), W(title), wxOK, state_->window.get());
}
bool ModuleActivity::Confirm(const std::string& text,
                             const std::string& title) {
  return wxMessageBox(W(text), W(title), wxYES_NO | wxICON_QUESTION,
                      state_->window.get()) == wxYES;
}
void ModuleActivity::PreviewText(const std::string& text,
                                 const std::string& title) {
  wxDialog dialog(state_->window.get(), wxID_ANY, W(title), wxDefaultPosition,
                  wxSize(480, 520));
  auto* control = new wxTextCtrl(&dialog, wxID_ANY, W(text), wxDefaultPosition,
                                 wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);
  control->SetFont(wxFontInfo(11).Family(wxFONTFAMILY_TELETYPE));
  auto* layout = new wxBoxSizer(wxVERTICAL);
  layout->Add(control, 1, wxEXPAND | wxALL, 12);
  layout->Add(dialog.CreateButtonSizer(wxOK), 0, wxEXPAND | wxALL, 12);
  dialog.SetSizer(layout);
  dialog.ShowModal();
}
}  // namespace uiutils
