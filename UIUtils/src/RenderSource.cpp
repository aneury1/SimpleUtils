#include <wx/image.h>
#include <wx/log.h>
#include "internal.h"
namespace uiutils {
bool Supports(ImageKind kind) {
  switch (kind) {
    case ImageKind::File:
      return true;
    case ImageKind::QrCode:
    case ImageKind::Barcode:
      return UIUTILS_ENABLE_CODES != 0;
    case ImageKind::Chart:
      return UIUTILS_ENABLE_CHARTS != 0;
    case ImageKind::PdfPage:
      return UIUTILS_ENABLE_PDF != 0;
  }
  return false;
}
namespace detail {
bool RenderSource(ElementState& state, const ImageSource& source, Size size,
                  wxBitmap& bitmap) {
  state.error.clear();
  if (size.width <= 0 || size.height <= 0 || size.width > 4096 ||
      size.height > 4096) {
    state.error = "Render size must be 1..4096 pixels";
    return false;
  }
  try {
    if (auto* file = std::get_if<FileImage>(&source.value)) {
      wxImage image;
      bool loaded = false;
      {
        wxLogNull quiet;
        if (!file->path.empty())
          loaded = image.LoadFile(W(file->path));
      }
      if (!loaded || !image.IsOk()) {
        state.error = "Image file could not be loaded";
        return false;
      }
      double ratio = std::min(double(size.width) / image.GetWidth(),
                              double(size.height) / image.GetHeight());
      image.Rescale(std::max(1, int(image.GetWidth() * ratio)),
                    std::max(1, int(image.GetHeight() * ratio)),
                    wxIMAGE_QUALITY_HIGH);
      bitmap = wxBitmap(image);
      return bitmap.IsOk();
    }
    if (std::holds_alternative<QrImage>(source.value) ||
        std::holds_alternative<BarcodeImage>(source.value))
      return RenderCodes(source, size, bitmap, state.error);
    if (auto* chart = std::get_if<ChartData>(&source.value))
      return RenderChart(*chart, size, bitmap, state.error);
    if (auto* pdf = std::get_if<PdfPageImage>(&source.value))
      return RenderPdf(state, *pdf, size, bitmap);
    state.error = "Unsupported image source";
  } catch (const std::exception& error) {
    state.error = error.what();
  }
  return false;
}
}  // namespace detail
}  // namespace uiutils
