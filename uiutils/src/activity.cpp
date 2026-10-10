#include "internal.h"
#include <wx/dialog.h>
namespace uiutils {
using namespace detail;
ActivityFrame::ActivityFrame(const std::string& title, Size size) : state_(std::make_unique<ActivityState>()) {
    auto* w = new wxMDIParentFrame(nullptr, wxID_ANY, W(title), wxDefaultPosition, wxDefaultSize);
    w->SetSize(w->FromDIP(WS(size)));
    state_->window = w; w->SetMenuBar(new wxMenuBar());
    state_->theme->Register(w, Role::Window, {});
    if (auto* client = w->GetClientWindow()) state_->theme->Register(client, Role::Window, {});
}
ActivityFrame::~ActivityFrame() {
    state_->modules.clear();
    if (state_->window) state_->window->Destroy();
}
void ActivityFrame::AddModule(std::unique_ptr<ModuleActivity> module) {
    auto& modules = state_->modules;
    // Wrappers remain alive until parent shutdown, even after native windows close.
    modules.push_back(std::move(module));
}
void ActivityFrame::SetTheme(Theme theme) { state_->theme->SetTheme(std::move(theme)); }
int ActivityFrame::addMenuBarOption(const std::string& label, int action) { return addMenuBarOption(label, label, action); }
int ActivityFrame::addMenuBarOption(const std::string& label, const std::string& item, int action) {
    if (!state_->window) throw std::logic_error("Activity is closed");
    auto* bar = state_->window->GetMenuBar(); int index = bar->FindMenu(W(label));
    wxMenu* menu;
    if (index == wxNOT_FOUND) { menu = new wxMenu(); bar->Append(menu, W(label)); }
    else menu = bar->GetMenu(index);
    return menu->Append(action, W(item))->GetId();
}
void ActivityFrame::OnMenu(int id, std::function<void()> callback) {
    state_->window->Bind(wxEVT_MENU, [callback = std::move(callback)](wxCommandEvent&) { if (callback) callback(); }, id);
}
void ActivityFrame::AddExitMenu(const std::string& label) { int id = addMenuBarOption(label, "E&xit"); OnMenu(id, [this] { Close(); }); }
void ActivityFrame::Show() { if (state_->window) state_->window->Show(); }
void ActivityFrame::Close() { if (state_->window) state_->window->Close(); }
ModuleActivity::ModuleActivity(ActivityFrame* parent, const std::string& title, Size size) : state_(std::make_unique<ModuleState>()) {
    if (!parent || !parent->state_->window) throw std::invalid_argument("Module requires an open parent");
    auto* w = new wxMDIChildFrame(parent->state_->window.get(), wxID_ANY, W(title), wxDefaultPosition, wxDefaultSize);
    w->SetSize(w->FromDIP(WS(size)));
    state_->window = w; state_->theme = parent->state_->theme; state_->parent = parent;
    state_->theme->Register(w, Role::Window, {});
}
ModuleActivity::~ModuleActivity() { if (state_->window) state_->window->Destroy(); }
bool ModuleActivity::IsOpen() const { return state_->window && !state_->closing && !state_->window->IsBeingDeleted(); }
void ModuleActivity::Show() { if (IsOpen()) state_->window->Show(); }
void ModuleActivity::Close() { if (IsOpen() && state_->window->Close()) state_->closing = true; }
void ModuleActivity::SetContent(const LayoutScope::Builder& build) {
    if (!IsOpen()) throw std::logic_error("Module is closed");
    if (state_->content) throw std::logic_error("SetContent can only be called once");
    state_->content = true;
    auto* w = state_->window.get(); auto* panel = new wxPanel(w);
    auto* root = new wxBoxSizer(wxVERTICAL); root->Add(panel, 1, wxEXPAND); w->SetSizer(root);
    auto* content = new wxBoxSizer(wxVERTICAL); panel->SetSizer(content);
    state_->theme->Register(panel, Role::Window, {});
    LayoutScope scope(std::make_shared<LayoutState>(LayoutState{panel, content, state_->theme})); build(scope); w->Layout();
}
void ModuleActivity::Message(const std::string& text, const std::string& title) {
    wxMessageBox(W(text), W(title), wxOK, state_->window.get());
}
bool ModuleActivity::Confirm(const std::string& text, const std::string& title) {
    return wxMessageBox(W(text), W(title), wxYES_NO | wxICON_QUESTION, state_->window.get()) == wxYES;
}
void ModuleActivity::PreviewText(const std::string& text, const std::string& title) {
    wxDialog dialog(state_->window.get(), wxID_ANY, W(title), wxDefaultPosition, wxSize(480, 520));
    auto* control = new wxTextCtrl(&dialog, wxID_ANY, W(text), wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);
    control->SetFont(wxFontInfo(11).Family(wxFONTFAMILY_TELETYPE));
    auto* layout = new wxBoxSizer(wxVERTICAL); layout->Add(control, 1, wxEXPAND | wxALL, 12);
    layout->Add(dialog.CreateButtonSizer(wxOK), 0, wxEXPAND | wxALL, 12); dialog.SetSizer(layout); dialog.ShowModal();
}
}

namespace uiutils {
void ActivityFrame::RegisterFrame(const std::string& name, FrameFactory factory) {
    if (name.empty() || !factory) throw std::invalid_argument("Frame name and factory are required");
    if (!state_->factories.emplace(name, std::move(factory)).second)
        throw std::invalid_argument("Frame name already registered: " + name);
}
ModuleActivity* ActivityFrame::GetCurrentFrame() const {
    if (!state_->window) return nullptr;
    auto* active = state_->window->GetActiveChild();
    for (const auto& module : state_->modules)
        if (module->IsOpen() && module->state_->window.get() == active) return module.get();
    return nullptr;
}
void ActivityFrame::Navigate(const std::string& name) {
    if (!state_->window) throw std::logic_error("Activity is closed");
    auto factory = state_->factories.find(name);
    if (factory == state_->factories.end()) throw std::invalid_argument("Unknown frame: " + name);
    if (state_->navigating) throw std::logic_error("Navigation during frame creation is unsupported");
    auto* current = GetCurrentFrame();
    if (current && current->state_->name == name) return;
    ModuleActivity* target = nullptr;
    for (const auto& module : state_->modules)
        if (module->IsOpen() && module->state_->name == name) target = module.get();
    if (!target) {
        state_->navigating = true;
        try {
            auto module = factory->second(this);
            if (!module || module->state_->parent != this || !module->IsOpen())
                throw std::invalid_argument("Factory must return an open module of this activity");
            module->state_->name = name;
            module->OnCreate();
            if (!module->IsOpen()) throw std::logic_error("Module closed during OnCreate");
            target = module.get();
            // Keep wrappers until parent shutdown: callbacks may still be unwinding.
            state_->modules.push_back(std::move(module));
            state_->navigating = false;
        } catch (...) { state_->navigating = false; throw; }
    }
    if (current && !current->state_->name.empty()) state_->history.push_back(current->state_->name);
    target->Show(); target->state_->window->Iconize(false); target->state_->window->Activate();
}
bool ActivityFrame::CanGoBack() const {
    auto* current = GetCurrentFrame();
    for (auto it = state_->history.rbegin(); it != state_->history.rend(); ++it)
        for (const auto& module : state_->modules)
            if (module->IsOpen() && module.get() != current && module->state_->name == *it) return true;
    return false;
}
bool ActivityFrame::GoBack() {
    auto* current = GetCurrentFrame();
    while (!state_->history.empty()) {
        auto name = state_->history.back(); state_->history.pop_back();
        for (const auto& module : state_->modules) {
            if (module->IsOpen() && module.get() != current && module->state_->name == name) {
                module->Show(); module->state_->window->Iconize(false); module->state_->window->Activate(); return true;
            }
        }
    }
    return false;
}
void ActivityFrame::CloseFrame(const std::string& name) {
    if (!state_->factories.count(name)) throw std::invalid_argument("Unknown frame: " + name);
    for (const auto& module : state_->modules)
        if (module->IsOpen() && module->state_->name == name) { module->Close(); break; }
    // GoBack filters closed frames, including closes through the native title bar.
}
void ModuleActivity::Navigate(const std::string& name) { state_->parent->Navigate(name); }
bool ModuleActivity::GoBack() { return state_->parent->GoBack(); }
bool ModuleActivity::CanGoBack() const { return state_->parent->CanGoBack(); }
}
