#include "internal.h"
#include <wx/dcmemory.h>
#include <sstream>
#include <iomanip>
namespace uiutils::detail {
bool RenderChart(const ChartData& chart, Size size, wxBitmap& bitmap, std::string& error) {
#if UIUTILS_ENABLE_CHARTS
    ValidateChart(chart);
    if (size.width < 240 || size.height < 160) { error = "Chart requires at least 240x160 pixels"; return false; }
    bitmap = wxBitmap(size.width, size.height);
    wxMemoryDC dc(bitmap); dc.SetBackground(*wxWHITE_BRUSH); dc.Clear();
    dc.SetFont(wxFont(wxFontInfo(11))); dc.SetTextForeground(*wxBLACK);
    dc.DrawText(W(chart.title), 60, 12);
    int left = 60, top = 42, width = size.width - 84, height = size.height - 98;
    auto limits = std::minmax_element(chart.values.begin(), chart.values.end());
    double lo = std::min(0.0, *limits.first), hi = std::max(0.0, *limits.second);
    if (lo == hi) hi = lo + 1;
    auto y = [&](double value) { return top + int((hi - value) / (hi - lo) * height); };
    dc.SetPen(wxPen(wxColour(220, 225, 230)));
    for (int i = 0; i <= 4; ++i) {
        int line = top + i * height / 4;
        dc.DrawLine(left, line, left + width, line);
        std::ostringstream label; label << std::setprecision(4) << hi - (hi - lo) * i / 4;
        dc.DrawText(W(label.str()), 4, line - 7);
    }
    dc.SetPen(*wxBLACK_PEN); dc.DrawLine(left, top, left, top + height); dc.DrawLine(left, y(0), left + width, y(0));
    wxColour color(W(chart.color.hex)); dc.SetPen(wxPen(color, 2)); dc.SetBrush(wxBrush(color));
    double step = double(width) / chart.values.size();
    int previousX = 0, previousY = 0;
    for (size_t i = 0; i < chart.values.size(); ++i) {
        int x = left + int((i + 0.5) * step), pointY = y(chart.values[i]);
        if (chart.type == ChartType::Bar) {
            int barWidth = std::max(1, int(step * 0.65));
            if (chart.values[i] != 0) dc.DrawRectangle(x - barWidth / 2, std::min(pointY, y(0)), barWidth, std::max(1, std::abs(y(0) - pointY)));
        } else {
            if (i) dc.DrawLine(previousX, previousY, x, pointY);
            dc.DrawCircle(x, pointY, 3); previousX = x; previousY = pointY;
        }
        // Thin labels for dense charts instead of overlapping every category.
        size_t stride = std::max<size_t>(1, (chart.values.size() + 9) / 10);
        if (i % stride == 0) {
            std::string text = chart.labels.empty() ? std::to_string(i + 1) : chart.labels[i];
            wxString label = W(text); if (label.length() > 10) label = label.Left(9) + "...";
            wxCoord tw, th; dc.GetTextExtent(label, &tw, &th);
            dc.DrawText(label, x - tw / 2, top + height + 10);
        }
    }
    dc.SelectObject(wxNullBitmap); return true;
#else
    (void)chart; (void)size; (void)bitmap;
    error = "Chart rendering is disabled; configure UIUTILS_ENABLE_CHARTS=ON"; return false;
#endif
}
}
