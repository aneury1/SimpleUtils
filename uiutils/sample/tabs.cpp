#include <uiutils/UiApplication.h>
using namespace uiutils;
void SettingsPage(LayoutScope& ui) {
    ui.Text("Functional settings component");
    ui.TextField("Restaurant name", Modifier{}.Expand().Margin(8));
    ui.CheckBox("Print receipts", true);
}
class TabsModule final : public ModuleActivity {
public:
    explicit TabsModule(ActivityFrame* parent) : ModuleActivity(parent, "Tabs gallery") {}
    void OnCreate() override {
        SetContent([](LayoutScope& ui) {
            auto status = ui.Text("Tab 0 selected");
            auto tabs = ui.Tabs({
                {"Orders", [](LayoutScope& page) { page.ListBox({"Table 1", "Table 2"}, 0, {}, Modifier{}.Weight().Expand()); }},
                {"Settings", [](LayoutScope& page) { page.Component(SettingsPage); }},
                {"Help", [](LayoutScope& page) { page.ScrollColumn([](LayoutScope& content) {
                    for (int i = 0; i < 40; ++i) content.Text("Help item " + std::to_string(i));
                }, Modifier{}.Weight().Expand()); }}
            }, 0, [status](int index) { status.SetText("Tab " + std::to_string(index) + " selected"); }, Modifier{}.Weight().Expand());
            ui.Row([tabs](LayoutScope& row) {
                row.Button("Settings (notify)", [tabs] { tabs.SetSelection(1, true); });
                row.Button("Rename orders", [tabs] { tabs.SetPageTitle(0, "Open orders"); });
                row.Button("Remove help", [tabs] { if (tabs.Count() > 2) tabs.RemovePage(2); });
            });
        });
    }
};
class Main final : public ActivityFrame {
public:
    Main() : ActivityFrame("Tabs demo") {}
    void OnCreate() override {
        RegisterFrame<TabsModule>("tabs"); Navigate("tabs"); AddExitMenu();
        int light = addMenuBarOption("&Theme", "Light"); OnMenu(light, [this] { SetTheme(LightTheme()); });
        int dark = addMenuBarOption("&Theme", "Dark"); OnMenu(dark, [this] { SetTheme(DarkTheme()); });
    }
};
int main(int argc, char** argv) { return RunApplication<Main>(argc, argv); }
