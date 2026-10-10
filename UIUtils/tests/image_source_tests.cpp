#include <gtest/gtest.h>
#include <uiutils/ImageSource.h>
#include <limits>
using namespace uiutils;
TEST(ImageSources, FactoriesPreservePayloadAndType) {
  EXPECT_EQ(std::get<QrImage>(ImageSource::Qr("hello").value).text, "hello");
  auto code = ImageSource::Barcode("123456789012", BarcodeType::Ean13);
  EXPECT_EQ(std::get<BarcodeImage>(code.value).type, BarcodeType::Ean13);
  EXPECT_EQ(std::get<PdfPageImage>(ImageSource::Pdf("file.pdf", 2).value).page,
            2);
  EXPECT_THROW(ImageSource::Pdf("file.pdf", -1), std::invalid_argument);
}
TEST(ImageSources, ChartsRejectBadDataAndAcceptSignedValues) {
  ChartData chart;
  EXPECT_THROW(ImageSource::Chart(chart), std::invalid_argument);
  chart.values = {-3, 0, 7};
  chart.labels = {"A", "B", "C"};
  EXPECT_NO_THROW(ImageSource::Chart(chart));
  chart.labels.pop_back();
  EXPECT_THROW(ImageSource::Chart(chart), std::invalid_argument);
  chart.labels.clear();
  chart.values[0] = std::numeric_limits<double>::quiet_NaN();
  EXPECT_THROW(ImageSource::Chart(chart), std::invalid_argument);
  chart.values = {-1e308, 1e308};
  EXPECT_THROW(ImageSource::Chart(chart), std::invalid_argument);
  chart.values.assign(129, 1);
  EXPECT_THROW(ImageSource::Chart(chart), std::invalid_argument);
}
