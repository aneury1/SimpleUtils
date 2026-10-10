#pragma once
#include <uiutils/Types.h>
#include <algorithm>
#include <cmath>
#include <variant>
namespace uiutils {
enum class ImageKind { File, QrCode, Barcode, Chart, PdfPage };
enum class BarcodeType { Code128, Ean13 };
enum class ChartType { Bar, Line };
struct FileImage {
  std::string path;
};
struct QrImage {
  std::string text;
};
struct BarcodeImage {
  std::string text;
  BarcodeType type = BarcodeType::Code128;
};
struct ChartData {
  std::string title;
  Items labels;
  std::vector<double> values;
  ChartType type = ChartType::Bar;
  Color color = Color("#1565C0");
};
inline void ValidateChart(const ChartData& chart) {
  if (chart.values.empty() || chart.values.size() > 128)
    throw std::invalid_argument("Chart requires 1..128 values");
  if (!chart.labels.empty() && chart.labels.size() != chart.values.size())
    throw std::invalid_argument(
        "Chart labels and values must have matching counts");
  for (double v : chart.values)
    if (!std::isfinite(v))
      throw std::invalid_argument("Chart values must be finite");
  auto limits = std::minmax_element(chart.values.begin(), chart.values.end());
  if (!std::isfinite(std::max(0.0, *limits.second) -
                     std::min(0.0, *limits.first)))
    throw std::invalid_argument("Chart range is too large");
  if (chart.type != ChartType::Bar && chart.type != ChartType::Line)
    throw std::invalid_argument("Invalid chart type");
}
struct PdfPageImage {
  std::string path;
  int page = 0;
};
class ImageSource {
 public:
  using Value =
      std::variant<FileImage, QrImage, BarcodeImage, ChartData, PdfPageImage>;
  Value value;

 private:
  explicit ImageSource(Value data) : value(std::move(data)) {}

 public:
  static ImageSource File(std::string path) {
    return ImageSource{FileImage{std::move(path)}};
  }
  static ImageSource Qr(std::string text) {
    return ImageSource{QrImage{std::move(text)}};
  }
  static ImageSource Barcode(std::string text,
                             BarcodeType type = BarcodeType::Code128) {
    return ImageSource{BarcodeImage{std::move(text), type}};
  }
  static ImageSource Chart(ChartData data) {
    ValidateChart(data);
    return ImageSource{std::move(data)};
  }
  static ImageSource Pdf(std::string path, int page = 0) {
    if (page < 0)
      throw std::invalid_argument("PDF page index must be nonnegative");
    return ImageSource{PdfPageImage{std::move(path), page}};
  }
};
// Build-time capability check; no dependency-specific types are exposed.
bool Supports(ImageKind kind);
}  // namespace uiutils
