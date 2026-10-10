#include <uiutils/UiApplication.h>
#include "Components.h"
using namespace uiutils;
ChartData SalesChart(ChartType type) {
  ChartData chart;
  chart.title = "Restaurant sales by day (EUR)";
  chart.labels = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
  chart.values = {230, 310, 275, 390, 580, 720, 610};
  chart.type = type;
  return chart;
}
class ChartsModule : public ModuleActivity {
 public:
  explicit ChartsModule(ActivityFrame* parent)
      : ModuleActivity(parent, "Charts", Size(900, 760)) {}
  void OnCreate() override {
    SetContent([](LayoutScope& ui) {
      ui.Component(components::Heading, "Bar and line charts");
      auto status =
          ui.Text("Charts use the internal renderer", Modifier{}.Margin(8));
      ui.ScrollColumn(
          [status](LayoutScope& column) {
            auto bars =
                column.Image(ImageSource::Chart(SalesChart(ChartType::Bar)),
                             Size(760, 300), Modifier{}.Margin(8));
            auto line =
                column.Image(ImageSource::Chart(SalesChart(ChartType::Line)),
                             Size(760, 300), Modifier{}.Margin(8));
            column.Button(
                "Update data",
                [bars, line, status] {
                  auto data = SalesChart(ChartType::Bar);
                  for (auto& value : data.values)
                    value += 45;
                  bool ok = bars.SetSource(ImageSource::Chart(data));
                  data.type = ChartType::Line;
                  ok = line.SetSource(ImageSource::Chart(data)) && ok;
                  status.SetText(ok ? "Updated both charts"
                                    : "Chart rendering failed");
                },
                Modifier{}.Margin(8));
            column.Button(
                "Save bar chart",
                [bars, status] {
                  status.SetText(
                      bars.SavePng("sales-chart.png")
                          ? "Saved sales-chart.png in current directory"
                          : "Chart could not be saved");
                },
                Modifier{}.Margin(8));
          },
          Modifier{}.Weight().Expand());
    });
  }
};
class ChartsApp : public ActivityFrame {
 public:
  ChartsApp() : ActivityFrame("uiutils - Chart sample") {}
  void OnCreate() override {
    AddExitMenu();
    OpenModule<ChartsModule>();
  }
};
int main(int argc, char** argv) {
  return RunApplication<ChartsApp>(argc, argv);
}
