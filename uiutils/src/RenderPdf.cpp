#include "internal.h"
#include <wx/image.h>
#include <cstring>
#if UIUTILS_ENABLE_PDF
#include <poppler-document.h>
#include <poppler-page.h>
#include <poppler-page-renderer.h>
#include <poppler-image.h>
#endif
namespace uiutils::detail {
struct PdfState {
    std::string path;
#if UIUTILS_ENABLE_PDF
    std::unique_ptr<poppler::document> document;
#endif
};
int PdfPageCount(const std::shared_ptr<PdfState>& state) {
#if UIUTILS_ENABLE_PDF
    return state && state->document ? state->document->pages() : 0;
#else
    (void)state; return 0;
#endif
}
bool RenderPdf(ElementState& state, const PdfPageImage& source, Size size, wxBitmap& bitmap) {
#if UIUTILS_ENABLE_PDF
    if (!state.pdf || state.pdf->path != source.path) {
        auto cache = std::make_shared<PdfState>(); cache->path = source.path;
        cache->document.reset(poppler::document::load_from_file(source.path));
        if (!cache->document) { state.error = "PDF could not be opened"; return false; }
        if (cache->document->is_locked()) { state.error = "Password-protected PDFs are not supported"; return false; }
        if (cache->document->pages() <= 0) { state.error = "PDF has no pages"; return false; }
        state.pdf = std::move(cache);
    }
    if (source.page < 0 || source.page >= state.pdf->document->pages()) { state.error = "PDF page is out of range"; return false; }
    std::unique_ptr<poppler::page> page(state.pdf->document->create_page(source.page));
    if (!page) { state.error = "PDF page could not be opened"; return false; }
    auto rect = page->page_rect();
    if (rect.width() <= 0 || rect.height() <= 0) { state.error = "Invalid PDF page dimensions"; return false; }
    double dpi = 72.0 * std::min(double(size.width) / rect.width(), double(size.height) / rect.height());
    poppler::page_renderer renderer;
    if (!renderer.can_render()) { state.error = "Poppler was built without page rendering support"; return false; }
    renderer.set_image_format(poppler::image::format_rgb24);
    auto rendered = renderer.render_page(page.get(), dpi, dpi);
    if (!rendered.is_valid() || rendered.width() <= 0 || rendered.height() <= 0 ||
        rendered.width() > 4096 || rendered.height() > 4096) { state.error = "PDF rendering failed or exceeded 4096 pixels"; return false; }
    wxImage image(rendered.width(), rendered.height());
    for (int y = 0; y < rendered.height(); ++y)
        std::memcpy(image.GetData() + size_t(y) * rendered.width() * 3,
                    rendered.const_data() + size_t(y) * rendered.bytes_per_row(), size_t(rendered.width()) * 3);
    bitmap = wxBitmap(image); return bitmap.IsOk();
#else
    (void)source; (void)size; (void)bitmap;
    state.error = "PDF rendering is disabled; configure UIUTILS_ENABLE_PDF=ON"; return false;
#endif
}
}
