#include <gtest/gtest.h>
#include "internal.h"
#include <wx/image.h>
#include <filesystem>
using namespace uiutils;
class TestApplication : public wxApp {
public: bool OnInit() override { wxInitAllImageHandlers(); return true; }
};
wxIMPLEMENT_APP_NO_MAIN(TestApplication);
class ElementsTest : public ::testing::Test {
protected:
    std::unique_ptr<ActivityFrame> parent;
    std::unique_ptr<ModuleActivity> module;
    void SetUp() override {
        parent = std::make_unique<ActivityFrame>("Test");
        module = std::make_unique<ModuleActivity>(parent.get(), "Test module");
    }
    void TearDown() override { module.reset(); parent.reset(); wxYield(); }
};
TEST_F(ElementsTest, FunctionalComponentReturnsAnElementAndSilentUpdatesDoNotNotify) {
    TextFieldElement input;
    int notifications = 0;
    auto field = [&notifications](LayoutScope& ui, const std::string& initial) {
        return ui.TextField(initial, {}, {}, [&notifications](const std::string&) { ++notifications; });
    };
    module->SetContent([&](LayoutScope& ui) { input = ui.Component(field, "Initial"); });
    EXPECT_EQ(input.GetValue(), "Initial");
    input.SetValue("Silent"); EXPECT_EQ(notifications, 0); EXPECT_EQ(input.GetValue(), "Silent");
    input.SetValue("Notified", true); EXPECT_EQ(notifications, 1);
}
TEST_F(ElementsTest, ListAndSpinHandlesValidateMutations) {
    ListBoxElement list; SpinElement spin;
    module->SetContent([&](LayoutScope& ui) {
        list = ui.ListBox({"A", "B"}, 0); spin = ui.SpinCtrl(1, 1, 99);
    });
    list.SetSelection(1); EXPECT_EQ(list.GetSelection(), 1);
    list.SetItems({"C"}); EXPECT_EQ(list.Count(), 1); EXPECT_EQ(list.GetSelection(), NoSelection);
    EXPECT_THROW(list.SetSelection(3), std::out_of_range);
    spin.SetValue(7); EXPECT_EQ(spin.GetValue(), 7);
    EXPECT_THROW(spin.SetValue(100), std::out_of_range);
    EXPECT_EQ(spin.GetValue(), 7);
}
TEST_F(ElementsTest, NativeClickInvokesThePublicCallback) {
    ButtonElement button; int clicks = 0;
    module->SetContent([&](LayoutScope& ui) { button = ui.Button("Click", [&] { ++clicks; }); });
    auto* native = detail::ElementAccess::Native(button);
    wxCommandEvent event(wxEVT_BUTTON, native->GetId()); event.SetEventObject(native);
    native->GetEventHandler()->ProcessEvent(event); EXPECT_EQ(clicks, 1);
}
TEST_F(ElementsTest, DeadElementsAreDetectedAndThemeSwitchSkipsThem) {
    TextElement text;
    module->SetContent([&](LayoutScope& ui) { text = ui.Text("Hello"); });
    ASSERT_TRUE(text.IsValid());
    delete detail::ElementAccess::Native(text);
    EXPECT_FALSE(text.IsValid()); EXPECT_THROW(text.SetText("Gone"), std::logic_error);
    EXPECT_NO_THROW(parent->SetTheme(DarkTheme()));
}
TEST_F(ElementsTest, LocalStyleAndEnabledStateSurviveThemeSwitch) {
    TextElement text;
    WidgetStyle style; style.Set<StyleKey::Foreground>(Color("#FF0000"));
    module->SetContent([&](LayoutScope& ui) { text = ui.Text("Hello", {}, style); });
    text.Enable(false); parent->SetTheme(DarkTheme());
    EXPECT_FALSE(text.IsEnabled());
    EXPECT_EQ(detail::ElementAccess::Native(text)->GetForegroundColour(), wxColour(255, 0, 0));
}
TEST_F(ElementsTest, ImageLoadsAPngAndMissingSourceUsesFallback) {
    ImageElement image;
    module->SetContent([&](LayoutScope& ui) { image = ui.Image(std::string{}, Size(32, 32)); });
    EXPECT_FALSE(image.SetSource("/missing/uiutils-image.png"));
    auto path = std::filesystem::temp_directory_path() / "uiutils-gtest-image.png";
    wxImage png(2, 2); png.SetRGB(wxRect(0, 0, 2, 2), 255, 0, 0);
    ASSERT_TRUE(png.SaveFile(detail::W(path.string()), wxBITMAP_TYPE_PNG));
    EXPECT_TRUE(image.SetSource(path.string())); std::filesystem::remove(path);
}
TEST_F(ElementsTest, ScrollContainersAndOneTimeContentContract) {
    ScrollElement scroll;
    module->SetContent([&](LayoutScope& ui) {
        scroll = ui.ScrollColumn([](LayoutScope& column) {
            for (int i = 0; i < 30; ++i) column.Text("Row " + std::to_string(i));
        }, Modifier{}.Weight().Expand());
    });
    EXPECT_NO_THROW(scroll.RefreshLayout());
    auto* native = static_cast<wxScrolledWindow*>(detail::ElementAccess::Native(scroll));
    EXPECT_GT(native->GetVirtualSize().y, 0);
    EXPECT_THROW(module->SetContent([](LayoutScope&) {}), std::logic_error);
}

TEST_F(ElementsTest, DisabledRenderersReturnActionableErrors) {
    ImageElement image;
    module->SetContent([&](LayoutScope& ui) { image = ui.Image(std::string{}, Size(300, 300)); });
    if (!Supports(ImageKind::QrCode)) {
        EXPECT_FALSE(image.SetSource(ImageSource::Qr("test")));
        EXPECT_NE(image.LastError().find("UIUTILS_ENABLE_CODES"), std::string::npos);
    }
    if (!Supports(ImageKind::PdfPage)) {
        EXPECT_FALSE(image.SetSource(ImageSource::Pdf("missing.pdf")));
        EXPECT_NE(image.LastError().find("UIUTILS_ENABLE_PDF"), std::string::npos);
    }
}
TEST_F(ElementsTest, EnabledChartsAndQrProduceExportableImages) {
    ImageElement image;
    module->SetContent([&](LayoutScope& ui) { image = ui.Image(std::string{}, Size(600, 300)); });
    auto path = std::filesystem::temp_directory_path() / "uiutils-render-test.png";
    if (Supports(ImageKind::Chart)) {
        ChartData data; data.values = {-3, 0, 7}; data.labels = {"A", "B", "C"};
        EXPECT_TRUE(image.SetSource(ImageSource::Chart(data)));
        EXPECT_TRUE(image.SavePng(path.string()));
        EXPECT_GT(std::filesystem::file_size(path), 0u); std::filesystem::remove(path);
    }
    if (Supports(ImageKind::QrCode)) {
        EXPECT_TRUE(image.SetSource(ImageSource::Qr("https://example.com")));
        EXPECT_TRUE(image.SavePng(path.string())); std::filesystem::remove(path);
        EXPECT_TRUE(image.SetSource(ImageSource::Barcode("ORDER-123")));
    }
}
TEST_F(ElementsTest, PdfViewerRendersAndNavigatesTheFixture) {
    if (!Supports(ImageKind::PdfPage)) GTEST_SKIP() << "PDF feature disabled";
    PdfElement viewer;
    module->SetContent([&](LayoutScope& ui) {
        ui.ScrollArea([&](LayoutScope& page) { viewer = page.PdfViewer(UIUTILS_TEST_PDF); });
    });
    ASSERT_EQ(viewer.PageCount(), 2) << viewer.LastError();
    EXPECT_EQ(viewer.CurrentPage(), 0);
    EXPECT_TRUE(viewer.GoToPage(1)); EXPECT_EQ(viewer.CurrentPage(), 1);
    EXPECT_FALSE(viewer.GoToPage(2)); EXPECT_EQ(viewer.CurrentPage(), 1);
    EXPECT_TRUE(viewer.SetZoom(1.25)); EXPECT_DOUBLE_EQ(viewer.GetZoom(), 1.25);
    EXPECT_FALSE(viewer.SetZoom(4));
}
class NavigationModule : public ModuleActivity {
public:
    explicit NavigationModule(ActivityFrame* parent) : ModuleActivity(parent, "Navigation test") {}
    void OnCreate() override { SetContent([](LayoutScope& ui) { ui.TextField("Retained"); }); }
};
TEST_F(ElementsTest, NavigationReusesFramesAndSkipsClosedHistory) {
    parent->Show();
    parent->RegisterFrame<NavigationModule>("a");
    parent->RegisterFrame<NavigationModule>("b");
    EXPECT_THROW(parent->RegisterFrame<NavigationModule>("a"), std::invalid_argument);
    EXPECT_THROW(parent->Navigate("missing"), std::invalid_argument);
    parent->Navigate("a"); wxYield();
    auto* first = parent->GetCurrentFrame(); ASSERT_NE(first, nullptr);
    parent->Navigate("b"); wxYield();
    EXPECT_NE(parent->GetCurrentFrame(), first);
    EXPECT_TRUE(parent->CanGoBack()); EXPECT_TRUE(parent->GoBack()); wxYield();
    EXPECT_EQ(parent->GetCurrentFrame(), first);
    parent->Navigate("b"); wxYield(); parent->CloseFrame("a"); wxYield();
    EXPECT_FALSE(parent->CanGoBack()); EXPECT_FALSE(parent->GoBack());
    parent->Navigate("a"); wxYield(); EXPECT_NE(parent->GetCurrentFrame(), first);
}
TEST_F(ElementsTest, TabsSupportSilentSelectionAndDestroyRemovedPageHandles) {
    TabsElement tabs; TextElement removed; int notifications = 0;
    module->SetContent([&](LayoutScope& ui) {
        tabs = ui.Tabs({{"A", [](LayoutScope& page) { page.Text("A"); }},
                        {"B", [&](LayoutScope& page) { removed = page.Text("B"); }}},
                       0, [&](int) { ++notifications; }, Modifier{}.Weight().Expand());
    });
    ASSERT_EQ(tabs.Count(), 2); EXPECT_EQ(tabs.GetSelection(), 0);
    tabs.SetSelection(1); EXPECT_EQ(tabs.GetSelection(), 1); EXPECT_EQ(notifications, 0);
    tabs.SetSelection(0, true); EXPECT_EQ(notifications, 1);
    tabs.SetPageTitle(0, "Orders"); EXPECT_EQ(tabs.GetPageTitle(0), "Orders");
    EXPECT_THROW(tabs.SetSelection(3), std::out_of_range);
    tabs.RemovePage(1); wxYield(); EXPECT_EQ(tabs.Count(), 1); EXPECT_FALSE(removed.IsValid());
    EXPECT_NO_THROW(parent->SetTheme(DarkTheme()));
    tabs.RemovePage(0); EXPECT_EQ(tabs.Count(), 0); EXPECT_EQ(tabs.GetSelection(), NoSelection);
}
TEST_F(ElementsTest, InvalidTabsAreRejectedBeforeBuilding) {
    module->SetContent([](LayoutScope& ui) {
        EXPECT_THROW(ui.Tabs({}), std::out_of_range);
        EXPECT_THROW(ui.Tabs({{"Missing", {}}}), std::invalid_argument);
    });
}
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    if (!wxEntryStart(argc, argv)) return 1;
    if (!wxTheApp->CallOnInit()) { wxEntryCleanup(); return 1; }
    int result = RUN_ALL_TESTS(); wxTheApp->OnExit(); wxEntryCleanup(); return result;
}
