#include <uiutils/UiApplication.h>
using namespace uiutils;
class Orders final : public ModuleActivity {
public:
    explicit Orders(ActivityFrame* parent) : ModuleActivity(parent, "Orders") {}
    void OnCreate() override {
        SetContent([this](LayoutScope& ui) {
            ui.Text("Orders: enter a note, switch frames, and return. Your note stays here.");
            ui.TextField("Table 1", Modifier{}.Expand().Margin(8));
            ui.Button("Products", [this] { Navigate("products"); });
            ui.Button("Reports", [this] { Navigate("reports"); });
            ui.Button("Back", [this] { if (!GoBack()) Message("No previous open frame"); });
        });
    }
};
class Products final : public ModuleActivity {
public:
    explicit Products(ActivityFrame* parent) : ModuleActivity(parent, "Products") {}
    void OnCreate() override {
        SetContent([this](LayoutScope& ui) {
            ui.ListBox({"Coffee", "Pizza", "Juice"}, 0, {}, Modifier{}.Weight().Expand());
            ui.Button("Orders", [this] { Navigate("orders"); });
            ui.Button("Back", [this] { GoBack(); });
        });
    }
};
class Reports final : public ModuleActivity {
public:
    explicit Reports(ActivityFrame* parent) : ModuleActivity(parent, "Reports") {}
    void OnCreate() override {
        SetContent([this](LayoutScope& ui) {
            ui.Tabs({{"Sales", [](LayoutScope& page) { page.Text("Demo sales: RD$ 2,500"); }},
                     {"Inventory", [](LayoutScope& page) { page.Text("Coffee: 30 units"); }}},
                     0, {}, Modifier{}.Weight().Expand());
            ui.Button("Back", [this] { GoBack(); });
        });
    }
};
class Main final : public ActivityFrame {
public:
    Main() : ActivityFrame("Frame navigation demo") {}
    void OnCreate() override {
        RegisterFrame<Orders>("orders"); RegisterFrame<Products>("products"); RegisterFrame<Reports>("reports");
        for (const std::string name : {"orders", "products", "reports"}) {
            int id = addMenuBarOption("&Navigate", name); OnMenu(id, [this, name] { Navigate(name); });
        }
        int back = addMenuBarOption("&Navigate", "Back"); OnMenu(back, [this] { GoBack(); });
        int close = addMenuBarOption("&Navigate", "Close products"); OnMenu(close, [this] { CloseFrame("products"); });
        int light = addMenuBarOption("&Theme", "Light"); OnMenu(light, [this] { SetTheme(LightTheme()); });
        int dark = addMenuBarOption("&Theme", "Dark"); OnMenu(dark, [this] { SetTheme(DarkTheme()); });
        AddExitMenu(); Navigate("orders");
    }
};
int main(int argc, char** argv) { return RunApplication<Main>(argc, argv); }
