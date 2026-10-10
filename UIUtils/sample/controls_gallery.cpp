#include <uiutils/Controls.h>
#include <uiutils/UiApplication.h>
#include "Components.h"
using namespace uiutils;
// A functional component can return several elements as application-specific data.
struct InputSection {
  TextFieldElement name;
  TextFieldElement notes;
};
InputSection InputFields(
    LayoutScope& ui, const std::function<void(const std::string&)>& report) {
  InputSection fields;
  ui.Column(
      [&](LayoutScope& column) {
        column.Component(components::Heading, "Text inputs");
        fields.name = column.TextField("Example", Modifier{}.Expand().Margin(8),
                                       {}, report);
        column.TextField({}, Modifier{}.Expand().Margin(8), {}, {},
                         TextInputMode::Password);
        column.TextField("Read-only value", Modifier{}.Expand().Margin(8), {},
                         {}, TextInputMode::ReadOnly);
        WidgetStyle style;
        style.Set<StyleKey::MinSize>(Size(300, 100));
        fields.notes =
            column.TextField("Multiline notes", Modifier{}.Expand().Margin(8),
                             style, report, TextInputMode::MultiLine);
      },
      Modifier{}.Expand());
  return fields;
}
class Gallery : public ModuleActivity {
 public:
  explicit Gallery(ActivityFrame* parent)
      : ModuleActivity(parent, "Element gallery", Size(760, 650)) {}
  void OnCreate() override {
    SetContent([this](LayoutScope& ui) {
      auto status = ui.Text("Ready", Modifier{}.Margin(8));
      auto report = [status](const std::string& text) {
        status.SetText(text);
      };
      ui.ScrollColumn(
          [this, report](LayoutScope& column) {
            auto full = Modifier{}.Expand().Margin(8);
            auto inputs = column.Component(InputFields, report);
            column.Button(
                "Read name",
                [this, inputs] { Message(inputs.name.GetValue()); }, full);
            column.Component(components::Heading, "Images");
            column.Image(
                std::string{}, Size(64, 64),
                Modifier{}.Margin(
                    8));  // Missing source uses an internal placeholder.
            column.CheckBox(
                "Notifications", true,
                [report](bool v) { report(v ? "Enabled" : "Disabled"); }, full);
            column.ToggleButton(
                "Active", false,
                [report](bool v) { report(v ? "Active" : "Inactive"); }, full);
            Items cities{"Madrid", "Santo Domingo", "New York"};
            column.Choice(
                cities, 0,
                [report, cities](int index) { report(cities.at(index)); },
                full);
            column.ComboBox(cities, "Madrid", report, full);
            column.RadioBox(
                "Priority", {"Low", "Normal", "High"}, 1,
                [report](int index) {
                  report("Priority: " + std::to_string(index));
                },
                full);
            WidgetStyle listStyle;
            listStyle.Set<StyleKey::MinSize>(Size(300, 100));
            column.ListBox(
                cities, 0,
                [report, cities](int index) { report(cities.at(index)); }, full,
                listStyle);
            auto progress = column.ProgressBar(100, 25, full);
            column.Slider(
                25, 0, 100,
                [progress, report](int value) {
                  progress.SetValue(value);
                  report("Progress: " + std::to_string(value));
                },
                full);
            column.SpinCtrl(
                1, 1, 99,
                [report](int value) {
                  report("Quantity: " + std::to_string(value));
                },
                full);
            column.Button(
                "Complete", [progress] { progress.SetValue(100); }, full);
            column.Separator(full);
            auto horizontal = column.ScrollRow(
                [report](LayoutScope& row) {
                  for (int i = 1; i <= 12; ++i)
                    row.Button(
                        "Action " + std::to_string(i),
                        [report, i] { report("Action " + std::to_string(i)); },
                        Modifier{}.Margin(8));
                },
                full);
            horizontal.SetMinSize(Size(1, 80));
            horizontal.RefreshLayout();
          },
          Modifier{}.Weight().Expand());
    });
  }
};
class MainActivity : public ActivityFrame {
 public:
  MainActivity() : ActivityFrame("uiutils element gallery") {}
  void OnCreate() override {
    AddExitMenu();
    int light = addMenuBarOption("&Appearance", "&Light theme");
    int dark = addMenuBarOption("&Appearance", "&Dark theme");
    OnMenu(light, [this] { SetTheme(LightTheme()); });
    OnMenu(dark, [this] { SetTheme(DarkTheme()); });
    OpenModule<Gallery>();
  }
};
int main(int argc, char** argv) {
  return RunApplication<MainActivity>(argc, argv);
}
