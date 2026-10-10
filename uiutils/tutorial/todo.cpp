#include <uiutils/UiApplication.h>
#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

using namespace uiutils;

struct Task {
    std::string title;
    bool done = false;
};

// A functional component returns non-owning element handles.
TextElement Heading(LayoutScope& ui, const std::string& title) {
    WidgetStyle style;
    style.Set<StyleKey::FontSize>(18).Set<StyleKey::Bold>(true);
    return ui.Text(title, Modifier{}.Margin(8), style);
}

class TodoActivity final : public ModuleActivity {
    std::vector<Task> tasks_;
    std::vector<std::size_t> visible_; // list row -> task index
    TextFieldElement input_;
    ListBoxElement list_;
    ChoiceElement filter_;
    TextElement summary_;
    ButtonElement toggle_;
    ButtonElement remove_;

    void UpdateActions() {
        const int row = list_.GetSelection();
        const bool selected = row >= 0 && static_cast<std::size_t>(row) < visible_.size();
        toggle_.Enable(selected);
        remove_.Enable(selected);
        toggle_.SetText(selected && tasks_[visible_[row]].done
            ? "Mark pending" : "Mark completed");
    }

    void RefreshTasks() {
        const int filter = filter_.GetSelection(); // 0 all, 1 pending, 2 completed
        Items labels;
        visible_.clear();
        std::size_t completed = 0;
        for (std::size_t i = 0; i < tasks_.size(); ++i) {
            const auto& task = tasks_[i];
            if (task.done) ++completed;
            if ((filter == 1 && task.done) || (filter == 2 && !task.done)) continue;
            visible_.push_back(i);
            labels.push_back(std::string(task.done ? "[Done] " : "[Pending] ") + task.title);
        }
        list_.SetItems(labels); // resets selection
        summary_.SetText(std::to_string(tasks_.size()) + " tasks | " +
            std::to_string(completed) + " completed | " +
            std::to_string(tasks_.size() - completed) + " pending");
        UpdateActions();
    }

    void AddTask() {
        std::string title = input_.GetValue();
        auto nonSpace = [](unsigned char ch) { return !std::isspace(ch); };
        auto first = std::find_if(title.begin(), title.end(), nonSpace);
        auto last = std::find_if(title.rbegin(), title.rend(), nonSpace).base();
        if (first >= last) { Message("Enter a task title.", "Todo"); return; }
        tasks_.push_back({std::string(first, last), false});
        input_.SetValue(""); // silent update
        RefreshTasks();
        input_.Focus();
    }

    void ToggleSelected() {
        const int row = list_.GetSelection();
        if (row < 0 || static_cast<std::size_t>(row) >= visible_.size()) return;
        auto& task = tasks_[visible_[row]];
        task.done = !task.done;
        RefreshTasks();
    }

    void RemoveSelected() {
        const int row = list_.GetSelection();
        if (row < 0 || static_cast<std::size_t>(row) >= visible_.size()) return;
        if (!Confirm("Delete this task?", "Todo")) return;
        tasks_.erase(tasks_.begin() + visible_[row]);
        RefreshTasks();
    }

public:
    explicit TodoActivity(ActivityFrame* parent) : ModuleActivity(parent, "Todo") {}

    void OnCreate() override {
        SetContent([this](LayoutScope& ui) {
            ui.Component(Heading, "My tasks");
            ui.Tabs({
                {"Tasks", [this](LayoutScope& page) {
                    page.Row([this](LayoutScope& row) {
                        input_ = row.TextField("", Modifier{}.Weight().Expand().Margin(8));
                        row.Button("Add task", [this] { AddTask(); }, Modifier{}.Margin(8));
                    }, Modifier{}.Expand());
                    filter_ = page.Choice({"All", "Pending", "Completed"}, 0,
                        [this](int) { RefreshTasks(); }, Modifier{}.Expand().Margin(8));
                    list_ = page.ListBox({}, NoSelection,
                        [this](int) { UpdateActions(); }, Modifier{}.Weight().Expand().Margin(8));
                    page.Row([this](LayoutScope& row) {
                        toggle_ = row.Button("Mark completed", [this] { ToggleSelected(); }, Modifier{}.Margin(8));
                        remove_ = row.Button("Delete", [this] { RemoveSelected(); }, Modifier{}.Margin(8));
                        row.Button("Clear completed", [this] {
                            tasks_.erase(std::remove_if(tasks_.begin(), tasks_.end(),
                                [](const Task& task) { return task.done; }), tasks_.end());
                            RefreshTasks();
                        }, Modifier{}.Margin(8));
                    }, Modifier{}.Expand());
                    summary_ = page.Text("", Modifier{}.Margin(8));
                }},
                {"Help", [](LayoutScope& page) {
                    page.Text("Add a task, select it, then mark it completed or delete it.", Modifier{}.Margin(8));
                    page.Text("Use the filter to show pending or completed tasks.", Modifier{}.Margin(8));
                    page.Text("Tasks stay in memory. Closing this module discards them.", Modifier{}.Margin(8));
                }}
            }, 0, {}, Modifier{}.Weight().Expand());
        });
        // Initialize after all handles have been assigned.
        RefreshTasks();
    }
};

class TodoApp final : public ActivityFrame {
public:
    TodoApp() : ActivityFrame("uiutils Todo", Size(1100, 800)) {}

    void OnCreate() override {
        RegisterFrame<TodoActivity>("todo");
        int open = addMenuBarOption("&File", "&Open Todo");
        OnMenu(open, [this] { Navigate("todo"); });
        AddExitMenu();
        int light = addMenuBarOption("&Appearance", "&Light theme");
        OnMenu(light, [this] { SetTheme(LightTheme()); });
        int dark = addMenuBarOption("&Appearance", "&Dark theme");
        OnMenu(dark, [this] { SetTheme(DarkTheme()); });
        Navigate("todo");
    }
};

int main(int argc, char** argv) {
    return RunApplication<TodoApp>(argc, argv);
}
