#include "internal.h"
#include <wx/image.h>
#if UIUTILS_ENABLE_CODES
#if __has_include(<MultiFormatWriter.h>)
#include <MultiFormatWriter.h>
#include <BarcodeFormat.h>
#include <BitMatrix.h>
#else
#include <ZXing/MultiFormatWriter.h>
#include <ZXing/BarcodeFormat.h>
#include <ZXing/BitMatrix.h>
#endif
#endif
namespace uiutils::detail {
bool RenderCodes(const ImageSource& source, Size size, wxBitmap& bitmap, std::string& error) {
#if UIUTILS_ENABLE_CODES
    bool qr = std::holds_alternative<QrImage>(source.value);
    std::string text;
    ZXing::BarcodeFormat format = ZXing::BarcodeFormat::QRCode;
    if (qr) text = std::get<QrImage>(source.value).text;
    else {
        const auto& barcode = std::get<BarcodeImage>(source.value);
        text = barcode.text;
        switch(barcode.type) {
        case BarcodeType::Code128: format = ZXing::BarcodeFormat::Code128; break;
        case BarcodeType::Ean13: format = ZXing::BarcodeFormat::EAN13; break;
        default: error = "Unsupported barcode type"; return false;
        }
    }
    if (text.empty() || text.size() > 4096) { error = "Code content must contain 1..4096 UTF-8 bytes"; return false; }
    ZXing::MultiFormatWriter writer(format);
    writer.setMargin(qr ? 4 : 20);
    if (qr) writer.setEncoding(ZXing::CharacterSet::UTF8);
    auto matrix = writer.encode(text, 0, qr ? 0 : 1);
    if (matrix.width() <= 0 || matrix.height() <= 0 || (!qr && size.height < 20)) {
        error = "Code dimensions are invalid or too small"; return false;
    }
    int scale = qr ? std::min(size.width / matrix.width(), size.height / matrix.height())
                   : size.width / matrix.width();
    if (scale < 1) { error = "Image is too small for this code; increase its dimensions"; return false; }
    wxImage image(size.width, size.height);
    image.SetRGB(wxRect(0, 0, size.width, size.height), 255, 255, 255);
    int left = (size.width - matrix.width() * scale) / 2;
    int top = qr ? (size.height - matrix.height() * scale) / 2 : 8;
    int height = qr ? scale : std::max(1, size.height - 16);
    for (int y = 0; y < matrix.height(); ++y) for (int x = 0; x < matrix.width(); ++x)
        if (matrix.get(x, y)) image.SetRGB(wxRect(left + x * scale, top + y * height, scale, height), 0, 0, 0);
    bitmap = wxBitmap(image); return bitmap.IsOk();
#else
    (void)source; (void)size; (void)bitmap;
    error = "QR/barcode rendering is disabled; configure UIUTILS_ENABLE_CODES=ON"; return false;
#endif
}
}
