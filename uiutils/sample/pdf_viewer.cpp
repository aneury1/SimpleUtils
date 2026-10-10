#include <uiutils/UiApplication.h>
#include "Components.h"
#include <filesystem>
using namespace uiutils;
static std::string initialPdf;
class PdfModule : public ModuleActivity {
    PdfElement document_;
    TextElement status_;
    void Status() {
        if (!document_.LastError().empty()) status_.SetText(document_.LastError());
        else status_.SetText("Page " + std::to_string(document_.CurrentPage() + 1) + " of " +
            std::to_string(document_.PageCount()) + " | " + std::to_string(int(document_.GetZoom() * 100)) + "%");
    }
public:
    explicit PdfModule(ActivityFrame* parent) : ModuleActivity(parent, "PDF viewer", Size(950, 820)) {}
    void OnCreate() override {
        SetContent([this](LayoutScope& ui) {
            ui.Component(components::Heading, "PDF page renderer");
            ui.Row([this](LayoutScope& row) {
                auto path = row.TextField(initialPdf, Modifier{}.Weight().Expand().Margin(8));
                row.Button("Open PDF", [this, path] { document_.Open(path.GetValue()); Status(); }, Modifier{}.Margin(8));
            }, Modifier{}.Expand());
            ui.Row([this](LayoutScope& row) {
                row.Button("Previous", [this] { document_.GoToPage(document_.CurrentPage() - 1); Status(); }, Modifier{}.Margin(6));
                row.Button("Next", [this] { document_.GoToPage(document_.CurrentPage() + 1); Status(); }, Modifier{}.Margin(6));
                row.Button("Zoom out", [this] { document_.SetZoom(document_.GetZoom() - 0.25); Status(); }, Modifier{}.Margin(6));
                row.Button("Zoom in", [this] { document_.SetZoom(document_.GetZoom() + 0.25); Status(); }, Modifier{}.Margin(6));
            }, Modifier{}.Expand());
            status_ = ui.Text("Open a PDF", Modifier{}.Margin(8));
            // Two-axis viewport lets zoomed pages scroll vertically and horizontally.
            ui.ScrollArea([this](LayoutScope& page) {
                document_ = page.PdfViewer(initialPdf, Size(612, 792), Modifier{}.Margin(8));
            }, Modifier{}.Weight().Expand());
            Status();
        });
    }
};
class PdfApp : public ActivityFrame {
public:
    PdfApp() : ActivityFrame("uiutils - PDF sample", Size(1100, 900)) {}
    void OnCreate() override { AddExitMenu(); OpenModule<PdfModule>(); }
};
int main(int argc, char** argv) {
    initialPdf = (std::filesystem::absolute(argv[0]).parent_path() / "assets/example.pdf").string();
    return RunApplication<PdfApp>(argc, argv);
}
