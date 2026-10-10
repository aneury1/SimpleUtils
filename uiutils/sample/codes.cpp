#include <uiutils/UiApplication.h>
#include "Components.h"
using namespace uiutils;
class CodesModule : public ModuleActivity {
public:
    explicit CodesModule(ActivityFrame* parent) : ModuleActivity(parent, "QR and barcodes", Size(850, 780)) {}
    void OnCreate() override {
        SetContent([](LayoutScope& ui) {
            ui.Component(components::Heading, "QR codes and barcodes");
            auto status = ui.Text("ZXing-C++ renderer", Modifier{}.Margin(8));
            auto full = Modifier{}.Expand().Margin(8);
            ui.ScrollColumn([status, full](LayoutScope& column) {
                auto qrText = column.TextField("https://example.com/menu", full);
                auto qr = column.Image(ImageSource::Qr(qrText.GetValue()), Size(280, 280), Modifier{}.Margin(8));
                column.Row([qr, qrText, status](LayoutScope& row) {
                    row.Button("Generate QR", [qr, qrText, status] {
                        status.SetText(qr.SetSource(ImageSource::Qr(qrText.GetValue())) ? "QR updated" : qr.LastError());
                    }, Modifier{}.Margin(8));
                    row.Button("Save QR PNG", [qr, status] {
                        status.SetText(qr.SavePng("qr-code.png") ? "Saved qr-code.png in current directory" : "QR could not be saved");
                    }, Modifier{}.Margin(8));
                }, Modifier{}.Expand());
                column.Separator(full);
                auto text = column.TextField("ORDER-000123", full);
                auto format = column.Choice({"Code 128", "EAN-13"}, 0, {}, full);
                auto barcode = column.Image(ImageSource::Barcode(text.GetValue()), Size(620, 140), Modifier{}.Margin(8));
                column.Button("Generate barcode", [barcode, text, format, status] {
                    auto type = format.GetSelection() == 1 ? BarcodeType::Ean13 : BarcodeType::Code128;
                    status.SetText(barcode.SetSource(ImageSource::Barcode(text.GetValue(), type)) ? "Barcode updated" : barcode.LastError());
                }, Modifier{}.Margin(8));
                column.Text("EAN-13: use 12 digits (checksum generated) or 13 valid digits.", full);
                column.Button("Save barcode PNG", [barcode, status] {
                    status.SetText(barcode.SavePng("barcode.png") ? "Saved barcode.png in current directory" : "Barcode could not be saved");
                }, Modifier{}.Margin(8));
            }, Modifier{}.Weight().Expand());
        });
    }
};
class CodesApp : public ActivityFrame {
public:
    CodesApp() : ActivityFrame("uiutils - QR and barcode sample") {}
    void OnCreate() override { AddExitMenu(); OpenModule<CodesModule>(); }
};
int main(int argc, char** argv) { return RunApplication<CodesApp>(argc, argv); }
