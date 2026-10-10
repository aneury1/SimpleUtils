#include <uiutils/UiApplication.h>
#include <uiutils/Controls.h>
#include <array>
#include <map>
#include <vector>
#include <algorithm>

using namespace uiutils;
#include "RestaurantModel.h"
#include "Components.h"
#include <filesystem>
using namespace restaurant;
static std::string logoPath;

class RestaurantPOS final : public ModuleActivity {
    OrderBook orders_;
    int receiptNumber_ = 1;
    ListBoxElement ticket_;
    SpinElement quantity_;
    TextFieldElement notes_;
    TextElement total_;
    TextElement status_;
    TextElement orderTitle_;
    ButtonElement remove_;
    ButtonElement pay_;
    ButtonElement card_;
    ButtonElement clear_;
    Order& Current() { return orders_.Current(); }
    std::string TableName() const { return orders_.TableName(); }
    std::int64_t Total() { return orders_.Total(); }
    void Status(const std::string& text) { status_.SetText(text); }
    void RefreshOrder(int selection = NoSelection) {
        Items rows;
        for (const auto& line : Current().lines) {
            const auto& product = products[line.product];
            rows.push_back(std::to_string(line.quantity) + "x " + product.name + "   " +
                     Money(line.quantity * product.cents));
        }
        ticket_.SetItems(rows);
        if (selection >= 0 && static_cast<size_t>(selection) < Current().lines.size())
            ticket_.SetSelection(selection);
        orderTitle_.SetText("Order - " + TableName());
        total_.SetText("Total: " + Money(Total()));
        notes_.SetValue(Current().notes);
        bool selected = ticket_.GetSelection() != NoSelection;
        quantity_.Enable(selected);
        remove_.Enable(selected);
        pay_.Enable(!Current().lines.empty());
        card_.Enable(!Current().lines.empty());
        clear_.Enable(!Current().lines.empty() || !Current().notes.empty());
        if (selected) quantity_.SetValue(Current().lines[ticket_.GetSelection()].quantity);
    }
    void AddProduct(size_t index) {
        int selected = orders_.Add(index);
        if (selected == NoSelection) { Status("Maximum quantity is 99"); return; }
        RefreshOrder(selected);
        Status("Added " + products[index].name);
    }
    std::string Receipt(const std::string& payment = {}) {
        std::string text = "UIUTILS RESTAURANT\nDEMO - NOT A FISCAL RECEIPT\n";
        text += "Receipt #" + std::to_string(receiptNumber_) + "\n";
        text += TableName() + "\n--------------------------------\n";
        for (const auto& line : Current().lines) {
            const auto& product = products[line.product];
            text += std::to_string(line.quantity) + "x " + product.name + "\n";
            text += "  " + Money(product.cents) + " each    " + Money(product.cents * line.quantity) + "\n";
        }
        text += "--------------------------------\nTOTAL: " + Money(Total()) + "\n";
        text += "No separate tax calculation configured.\n";
        if (!Current().notes.empty()) text += "Notes: " + Current().notes + "\n";
        if (!payment.empty()) text += "Payment (simulated): " + payment + "\n";
        return text;
    }
    void Preview(const std::string& receipt) { PreviewText(receipt, "Receipt preview"); }
    void Pay(const std::string& method) {
        if (Current().lines.empty()) return;
        std::string prompt = "Record a simulated " + method + " payment of " + Money(Total()) + "?";
        if (!Confirm(prompt, "Demo checkout")) return;
        Preview(Receipt(method));
        ++receiptNumber_;
        orders_.Clear();
        RefreshOrder();
        Status("Payment recorded for " + TableName() + " (demo)");
    }
public:
    explicit RestaurantPOS(ActivityFrame* parent)
        : ModuleActivity(parent, "Restaurant POS", Size(1040, 720)) {}
    void OnCreate() override {
        SetContent([this](LayoutScope& ui) {
            WidgetStyle titleStyle;
            titleStyle.Set<StyleKey::FontSize>(18).Set<StyleKey::Bold>(true);
            ui.Row([](LayoutScope& row) {
                row.Image(logoPath, Size(64, 64), Modifier{}.Margin(8));
                row.Component(components::Heading, "Restaurant POS");
            }, Modifier{}.Expand());
            ui.Row([this](LayoutScope& row) {
                row.Text("Service:", Modifier{}.Margin(8));
                Items tables;
                for (int i = 1; i <= 6; ++i) tables.push_back("Table " + std::to_string(i));
                tables.push_back("Takeaway");
                row.Choice(tables, 0, [this](int index) {
                    orders_.SelectTable(index); RefreshOrder(); Status("Selected " + TableName());
                }, Modifier{}.Margin(8));
                row.Stretch();
                row.Text("EUR | In-memory demo", Modifier{}.Margin(8));
            }, Modifier{}.Expand());
            ui.Separator(Modifier{}.Expand().Margin(8));
            ui.Row([this, titleStyle](LayoutScope& body) {
                body.Column([this, titleStyle](LayoutScope& menu) {
                    menu.Text("Menu", Modifier{}.Margin(8), titleStyle);
                    menu.ScrollColumn([this](LayoutScope& content) {
                        const std::string categories[] = {"Main dishes", "Sides", "Drinks", "Desserts"};
                        WidgetStyle categoryStyle;
                        categoryStyle.Set<StyleKey::Bold>(true).Set<StyleKey::FontSize>(13);
                        for (const auto& category : categories) {
                            content.Text(category, Modifier{}.Margin(8), categoryStyle);
                            std::vector<size_t> matching;
                            for (size_t i = 0; i < products.size(); ++i)
                                if (products[i].category == category) matching.push_back(i);
                            for (size_t first = 0; first < matching.size(); first += 2) {
                                content.Row([this, &matching, first](LayoutScope& row) {
                                    for (size_t j = first; j < matching.size() && j < first + 2; ++j) {
                                        size_t index = matching[j];
                                        row.Component(components::ProductCard, products[index],
                                            [this, index] { AddProduct(index); });
                                    }
                                    if (matching.size() - first == 1) row.Stretch();
                                }, Modifier{}.Expand());
                            }
                        }
                    }, Modifier{}.Weight().Expand());
                }, Modifier{}.Weight(3).Expand().Margin(8));
                body.Column([this, titleStyle](LayoutScope& order) {
                    orderTitle_ = order.Text("Order", Modifier{}.Margin(8), titleStyle);
                    WidgetStyle listStyle;
                    listStyle.Set<StyleKey::MinSize>(Size(320, 180));
                    ticket_ = order.ListBox(Items{}, NoSelection,
                        [this](int index) {
                            if (index == NoSelection) return;
                            quantity_.SetValue(Current().lines[index].quantity);
                            quantity_.Enable(); remove_.Enable();
                        }, Modifier{}.Weight().Expand().Margin(8), listStyle);
                    order.Row([this](LayoutScope& row) {
                        row.Text("Quantity", Modifier{}.Margin(8));
                        quantity_ = row.SpinCtrl(1, 1, 99, [this](int value) {
                            int selected = ticket_.GetSelection();
                            if (selected == NoSelection) return;
                            orders_.SetQuantity(selected, value);
                            RefreshOrder(selected);
                        }, Modifier{}.Margin(8));
                        remove_ = row.Button("Remove", [this] {
                            int selected = ticket_.GetSelection();
                            if (selected == NoSelection) return;
                            orders_.Remove(selected); RefreshOrder();
                            Status("Item removed");
                        }, Modifier{}.Margin(8));
                    }, Modifier{}.Expand());
                    order.Text("Kitchen notes", Modifier{}.Margin(8));
                    WidgetStyle noteStyle;
                    noteStyle.Set<StyleKey::MinSize>(Size(320, 70));
                    notes_ = order.TextField({}, Modifier{}.Expand().Margin(8), noteStyle,
                        [this](const std::string& value) {
                            Current().notes = value;
                            clear_.Enable(!Current().lines.empty() || !value.empty());
                        }, TextInputMode::MultiLine);
                    total_ = order.Text("Total: EUR 0.00", Modifier{}.Margin(8), titleStyle);
                    order.Row([this](LayoutScope& row) {
                        pay_ = row.Button("Cash", [this] { Pay("cash"); }, Modifier{}.Weight().Margin(6));
                        card_ = row.Button("Card", [this] { Pay("card"); }, Modifier{}.Weight().Margin(6));
                        // Card payments are simulated; no terminal integration.
                        card_.SetTooltip("Simulated card payment");
                    }, Modifier{}.Expand());
                    order.Row([this](LayoutScope& row) {
                        row.Button("Receipt", [this] {
                            if (Current().lines.empty()) { Status("Add an item first"); return; }
                            Preview(Receipt());
                        }, Modifier{}.Weight().Margin(6));
                        clear_ = row.Button("Clear order", [this] {
                            if (Confirm("Clear this table's order?", "Clear order")) {
                                orders_.Clear(); RefreshOrder(); Status("Order cleared");
                            }
                        }, Modifier{}.Weight().Margin(6));
                    }, Modifier{}.Expand());
                }, Modifier{}.Weight(2).Expand().Margin(8));
            }, Modifier{}.Weight().Expand());
            status_ = ui.Text("Ready - select a menu item", Modifier{}.Margin(12));
            RefreshOrder();
        });
    }
};

class MainActivity final : public ActivityFrame {
public:
    MainActivity() : ActivityFrame("uiutils - Restaurant POS", Size(1200, 850)) {}
    void OnCreate() override {
        int open = addMenuBarOption("&File", "&New POS workspace");
        AddExitMenu();
        int light = addMenuBarOption("&Appearance", "&Light theme");
        int dark = addMenuBarOption("&Appearance", "&Dark theme");
        OnMenu(open, [this] { OpenModule<RestaurantPOS>(); });
        OnMenu(light, [this] { SetTheme(LightTheme()); });
        OnMenu(dark, [this] { SetTheme(DarkTheme()); });
        OpenModule<RestaurantPOS>();
    }
};
int main(int argc, char** argv) {
    logoPath = (std::filesystem::absolute(argv[0]).parent_path() / "assets/restaurant.png").string();
    return RunApplication<MainActivity>(argc, argv);
}
