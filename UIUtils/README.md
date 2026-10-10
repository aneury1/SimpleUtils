# uiutils 0.6.0
C++17 static library with a private wxWidgets backend. Application headers and samples
contain no native wxWidgets types. This is an API-breaking update from 0.4.0.

## Build
Requires CMake 3.20+, a C++17 compiler and wxWidgets GUI/MDI support (core/base).
```sh
cmake -S . -B build
cmake --build build --parallel
./build/sample/uiutils_sample
./build/sample/uiutils_controls_gallery
```
The logo is copied next to the POS executable. Launch using its filesystem path.
Images support formats enabled by the installed wxWidgets build; handlers initialize at startup.
Missing images show a built-in placeholder. Image.SetSource returns whether loading succeeded.

## Elements
LayoutScope returns value handles, never native pointers:
TextElement, ButtonElement, TextFieldElement, CheckBoxElement, ToggleElement,
ChoiceElement, ComboBoxElement, RadioElement, SliderElement, SpinElement,
ProgressElement, ListBoxElement, ImageElement and ScrollElement.
Elements own shared handle state, not their windows. Windows belong to their backend parents.
Handles are copyable; capture them by value in callbacks. IsValid detects destroyed windows;
operations on empty/destroyed handles throw logic_error. Use only on the GUI thread.
LayoutScope is for synchronous construction: do not retain it or capture it for later UI events.
ActivityFrame owns modules opened through OpenModule<T>(); closed module wrappers are pruned
when another module opens and remaining wrappers are released at application shutdown.

```cpp
auto label = ui.Text("Ready");
auto list = ui.ListBox({"Coffee", "Tea"});
auto quantity = ui.SpinCtrl(1, 1, 99);
ui.Button("Read", [label, list, quantity] {
    label.SetText("Selected " + std::to_string(list.GetSelection()) +
                  ", quantity " + std::to_string(quantity.GetValue()));
});
auto image = ui.Image("logo.png", uiutils::Size(96, 96));
```
Types: std::string (UTF-8), Items (vector<string>), Size (DIP), Color (#RRGGBB).
Use NoSelection, AutoId and enum StyleKey. Font size uses points.
WidgetStyle::Set<Key> checks value types at compilation; Set(key,value) validates at runtime.
Theme defaults merge with local overrides. Element Enable/Show/SetMinSize/ApplyStyle persist
through theme switches. Styles applied to one element do not recursively style children.
Native themes may limit control colors. Dialog helpers currently use native system styling.

## Functional components
A component is a regular function taking LayoutScope& and application parameters.
It can return a handle or a struct of handles. See sample/Components.h and InputFields in the gallery.
```cpp
uiutils::TextElement Heading(uiutils::LayoutScope& ui, const std::string& title) {
    return ui.Text(title);
}
auto heading = ui.Component(Heading, "Order");
```
Construction happens once. There is no state observation, remember(), or automatic recomposition.
Update existing elements through handles. TextField.SetValue is silent by default;
SetValue(value,true) requests native text notifications. Other setters follow native event rules.
SpinCtrl callbacks observe committed numeric changes. ScrollElement.RefreshLayout recalculates scrolling.

## Restaurant sample
Tables and takeaway have separate in-memory orders. Products merge into quantity lines,
notes are retained per table, integer-cent totals avoid floating-point rounding, and payment
is simulated with a receipt preview. No tax, fiscal, printer, persistence or payment integration.
Closing a workspace loses its orders. Workspaces have independent receipts and numbering.
The original controls gallery remains a separate executable.

## GoogleTest
By default no tests or downloads are enabled. Install GoogleTest or explicitly enable fetching:
```sh
cmake -S . -B build -DUIUTILS_BUILD_TESTS=ON -DUIUTILS_FETCH_GTEST=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```
Headless unit tests do not need wxWidgets:
```sh
cmake -S . -B build-unit -DUIUTILS_BUILD_LIBRARY=OFF -DUIUTILS_BUILD_SAMPLE=OFF \
  -DUIUTILS_BUILD_TESTS=ON -DUIUTILS_FETCH_GTEST=ON
cmake --build build-unit --parallel
ctest --test-dir build-unit --output-on-failure
```
GUI integration tests require wxWidgets and a display:
```sh
cmake -S . -B build -DUIUTILS_BUILD_TESTS=ON -DUIUTILS_BUILD_GUI_TESTS=ON -DUIUTILS_FETCH_GTEST=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
# For headless Linux with Xvfb installed:
xvfb-run -a ctest --test-dir build --output-on-failure
```
Unit tests cover style precedence/validation, immutable modifiers, order totals, quantity limits,
invalid mutations and table isolation. GUI tests cover functional returns, callbacks, silent updates,
handle lifetime, local styles during theme switching, image loading, scrolling and SetContent's contract.
Only backend integration tests include native wxWidgets headers; samples and public headers do not.

## Optional render components
| CMake option | Default | Dependency |
| --- | --- | --- |
| UIUTILS_ENABLE_CODES | OFF | ZXing-C++ 2.x with writers |
| UIUTILS_FETCH_ZXING | OFF | Explicit network fetch of pinned v2.3.0 |
| UIUTILS_ENABLE_CHARTS | ON | Internal renderer; no additional library |
| UIUTILS_ENABLE_PDF | OFF | Poppler C++ installed through pkg-config |

Disabled features do not require their dependencies. Their public API remains available:
Supports(ImageKind) reports availability and SetSource returns false with LastError explaining
which feature to enable. Poppler is system-only; it is not downloaded automatically.
If fetching ZXing is enabled, the pinned source is used instead of a system installation.
On systems with newer ZXing releases, enable fetching to select the supported 2.3.0 writer API.
Poppler must include its C++ interface and page renderer. Configure dependency prefixes with
CMAKE_PREFIX_PATH and/or PKG_CONFIG_PATH. Review upstream dependency licenses before distribution.

```sh
cmake -S . -B build -DUIUTILS_ENABLE_CODES=ON -DUIUTILS_FETCH_ZXING=ON \
  -DUIUTILS_ENABLE_CHARTS=ON -DUIUTILS_ENABLE_PDF=ON
cmake --build build --parallel
./build/sample/uiutils_codes_sample
./build/sample/uiutils_charts_sample
./build/sample/uiutils_pdf_sample
```
Corresponding sample targets are added only when their feature is enabled.
The PDF sample includes a two-page example and accepts another file path through its UI.
The POS and original element gallery are still included.

```cpp
auto qr = ui.Image(uiutils::ImageSource::Qr("https://example.com"), uiutils::Size(280, 280));
auto bar = ui.Image(uiutils::ImageSource::Barcode("ORDER-123"), uiutils::Size(600, 140));
uiutils::ChartData chart;
chart.title = "Sales"; chart.labels = {"Mon", "Tue"}; chart.values = {120, 180};
auto plot = ui.Image(uiutils::ImageSource::Chart(chart), uiutils::Size(600, 300));
auto pdf = ui.PdfViewer("report.pdf");
pdf.GoToPage(1); // zero-based page index
pdf.SetZoom(1.25);
```
ImageSource supports files, QR, Code128/EAN13, bar/line charts and PDF page snapshots.
ImageElement.SetSource accepts any source; LastError reports errors and SavePng exports the
current successfully rendered bitmap. Missing/failed sources show a placeholder.
Generated codes use integer module scaling with quiet zones and no smoothing. If a code
cannot fit, increase the image size. EAN13 requires valid digits/checksum; ZXing validates it.
Charts support one series, signed finite values, a title and category labels, up to 128 values.
The chart canvas is white and does not inherit the application's dark background.
PdfElement caches an opened document, renders one page at a time and supports 25%-300% zoom.
ScrollArea provides two-axis scrolling for zoomed pages. Rendering is synchronous on the GUI
thread. PDF output is rasterized; text selection, annotations and password entry are not implemented.
Render dimensions are limited to 4096 pixels per axis (including display scaling).
Public headers and samples still contain no native wxWidgets or third-party types.

## Validation during generation
Public headers and all five samples pass g++ C++17 syntax checks without wxWidgets installed.
A dependency-free smoke executable checks order invariants and style behavior.
CMake, wxWidgets, ZXing-C++, Poppler C++ and GoogleTest were unavailable, so backend compilation and GoogleTest/GUI execution
were not performed. Archive integrity and whitespace checks passed.

The bundled PDF was rendered with Poppler utilities and visually checked.
