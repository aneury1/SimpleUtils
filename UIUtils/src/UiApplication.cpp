#include <wx/image.h>
#include "internal.h"
namespace uiutils {
bool UIApplication::OnInit() {
  if (!wxApp::OnInit())
    return false;
  wxInitAllImageHandlers();
  activity_ = factory_();
  if (!activity_)
    return false;
  SetTopWindow(activity_->state_->window.get());
  activity_->OnCreate();
  activity_->Show();
  return true;
}
int UIApplication::OnExit() {
  activity_.reset();
  return wxApp::OnExit();
}
int RunApplication(int argc, char** argv, ActivityFactory factory) {
  if (wxApp::GetInstance())
    throw std::logic_error("Application already exists");
  wxApp::SetInstance(new UIApplication(std::move(factory)));
  return wxEntry(argc, argv);
}
}  // namespace uiutils
