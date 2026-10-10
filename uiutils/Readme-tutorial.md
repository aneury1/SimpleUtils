# uiutils tutorial: build a Todo application

This tutorial targets **uiutils 0.7.0** and builds a desktop Todo app using the
framework's public API. Application code uses standard C++ and uiutils types;
wxWidgets remains inside the library backend.

The finished app adds tasks, marks them completed or pending, deletes tasks,
filters the list, clears completed tasks, and switches between light and dark
themes. It has Tasks and Help tabs. Tasks are stored in memory only.

## 1. Prepare the project

You need a C++17 compiler, CMake 3.20 or newer, and a wxWidgets development
installation with the core/base components and MDI support. Use your existing
uiutils 0.7.0 project. Optional QR, barcode, chart, and PDF renderers are not
needed for this tutorial.

Create `sample/todo.cpp` and paste the complete source from section 7 into it.
Add these lines to the existing `sample/CMakeLists.txt`:

```cmake
add_executable(uiutils_todo_sample todo.cpp)
target_link_libraries(uiutils_todo_sample PRIVATE uiutils::uiutils)
```

The library target already supplies the C++17 requirement, public include path,
and link dependencies. You do not need to install uiutils system-wide when
building the sample inside its source tree.

From the uiutils project root, configure and build:

```bash
cmake -S . -B build -DUIUTILS_BUILD_SAMPLE=ON
cmake --build build --target uiutils_todo_sample --parallel
```

On Linux, run:

```bash
./build/sample/uiutils_todo_sample
```

For a Windows multi-configuration generator, build with `--config Release` and
run `build/sample/Release/uiutils_todo_sample.exe`. Executable paths may differ
with your generator.

## 2. Understand the application structure

| API | Responsibility in this app |
| --- | --- |
| `RunApplication<TodoApp>()` | Initializes the backend and runs the event loop |
| `ActivityFrame` | Main MDI workspace, menus, and application theme |
| `ModuleActivity` | Todo child window and its task state |
| `RegisterFrame<T>()` | Registers the factory for a named module |
| `Navigate()` | Creates or activates that module |
| `SetContent()` | Builds a module's controls once |
| `LayoutScope` | Creates elements and nested layouts |
| Element handles | Read or update existing controls from callbacks |

`TodoApp::OnCreate()` registers the Todo module, creates menu actions, and calls
`Navigate("todo")`. The navigator constructs `TodoActivity` with its parent,
calls the module's `OnCreate()`, and shows its window.

```cpp
RegisterFrame<TodoActivity>("todo");
Navigate("todo");
```

Navigating to an existing open module reuses it, preserving task state. Closing
it and choosing File → Open Todo creates a fresh, empty module. Closing the main
workspace ends the application.

For multiple screens, register more `ModuleActivity` subclasses under different
names. A module can call `Navigate("other")`, `GoBack()`, and `CanGoBack()`.
`GoBack()` returns false when no previous open module exists. This app uses one
module and tabs for its two views.

## 3. Compose layouts and styles

`SetContent()` receives a synchronous builder. Its root layout is a vertical
column. `Row()` arranges elements horizontally; `Column()` creates a nested
vertical layout.

```cpp
ui.Row([](uiutils::LayoutScope& row) {
    row.TextField("", uiutils::Modifier{}.Weight().Expand().Margin(8));
    row.Button("Add task");
}, uiutils::Modifier{}.Expand());
```

| Modifier | Meaning |
| --- | --- |
| `Weight(1)` | Receives a share of extra space on the layout's main axis |
| `Expand()` | Fills the cross axis |
| `Margin(8)` | Adds an outside border of 8 DIP |
| `Align(Alignment::Center)` | Centers on the cross axis when not expanded |

In the input row, the text field stretches horizontally while the button keeps
its natural width. In the Tasks page, the list uses `Weight().Expand()` to fill
available space. Margins use device-independent pixels; font sizes use points.

Styling is separate from layout. Use enum keys and the typed setter:

```cpp
uiutils::WidgetStyle headingStyle;
headingStyle.Set<uiutils::StyleKey::FontSize>(18)
            .Set<uiutils::StyleKey::Bold>(true);
```

The theme provides default styles and local styles override matching properties.
Changing themes updates registered controls while preserving those overrides.
Native tab headers, menus, and window decorations follow platform rendering
rules.

## 4. Use functional components and tabs

A functional component is a regular function that accepts `LayoutScope&`. It
may return an element handle or a struct of handles. The tutorial's `Heading()`
function creates a styled label:

```cpp
ui.Component(Heading, "My tasks");
```

Components build controls once; uiutils does not automatically recompose them
when application state changes.

`Tabs()` receives page titles and page builders. Each builder receives a scope
whose controls belong to that page:

```cpp
ui.Tabs({
    {"Tasks", [](uiutils::LayoutScope& page) { page.Text("Task controls"); }},
    {"Help", [](uiutils::LayoutScope& page) { page.Text("Instructions"); }}
}, 0, {}, uiutils::Modifier{}.Weight().Expand());
```

Index 0 selects the first page. The third argument is an optional selection
callback. Switching tabs preserves their controls and values.

A returned `TabsElement` also supports `Count()`, `GetSelection()`,
`SetSelection(index, notify)`, `GetPageTitle()`, `SetPageTitle()`, and
`RemovePage()`. Indices are zero-based. Selection changes are silent by default;
pass `true` to request notification. Removing a page destroys its controls and
invalidates their handles.

## 5. Separate state from the view

The model is a vector of small task records:

```cpp
struct Task {
    std::string title;
    bool done = false;
};
```

The UI holds `TextFieldElement`, `ListBoxElement`, `ChoiceElement`,
`TextElement`, and `ButtonElement` handles. It does not store native widget
pointers.

`RefreshTasks()` converts the model into list labels and updates the summary.
Filtering means a displayed row may refer to a different model index, so
`visible_` maps each list row to its underlying task. Toggle and delete actions
use that mapping. This prevents an action from changing the wrong task when a
filter is active.

List replacement clears the selection. The sample therefore disables Toggle
and Delete until the user selects a row again. Selection callbacks update the
button state without rebuilding the layout.

## 6. Wire callbacks and updates

Buttons call member functions that mutate the model, then refresh existing
controls:

```cpp
row.Button("Add task", [this] { AddTask(); });
```

The Add action trims surrounding whitespace, rejects blank titles, appends a
pending task, clears the input, and restores input focus. The Delete action
asks for confirmation through the module's `Confirm()` helper.

`input_.SetValue("")` is silent by default. It does not trigger a text-change
callback. Explicitly request notification with `SetValue(value, true)` when
needed.

Build all element handles before calling `RefreshTasks()`. The sample calls it
after `SetContent()` returns, so callbacks and refresh logic can safely use the
complete view.

Element handles are copyable and non-owning with respect to native windows.
Capture a handle by value in a callback when appropriate. The callbacks here
capture `this`; the module owns its state and its controls belong to that
module. Use the API on the GUI thread. Do not retain a `LayoutScope` or capture
it for later events. Operations on empty or destroyed handles throw; check
`IsValid()` if a handle may outlive its control.

## 7. Complete Todo source

Copy the entire block below into `sample/todo.cpp`. It includes the model,
functional component, module, menus, theme switching, and normal `main()`.

```cpp
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
```

## 8. Try the application

1. Add `Buy groceries`, `Read a chapter`, and `Review project`.
2. Select `Read a chapter` and click Mark completed.
3. Select Pending in the filter: only unfinished tasks should appear.
4. Select Completed, select the completed task, and click Mark pending.
   It disappears from this filtered view; select All to see it again.
5. Select a task, click Delete, and confirm. Cancel should leave it unchanged.
6. Complete another task and click Clear completed.
7. Switch between Tasks and Help; the task state should remain intact.
8. Use Appearance → Dark theme and Light theme.
9. Choose File → Open Todo while the module is open; it should activate the
   existing module. Close the Todo child and reopen it; its list should be empty.

An empty task title displays a message. Toggle and Delete are disabled without
a selection. Changing filters or modifying tasks resets the list selection.

## 9. Extend it into your own application

For persistent storage, load task records before the initial `RefreshTasks()`
and save them after successful model changes. Choose a writable user-data path,
handle file errors, and use stable task identifiers if tasks gain relationships.
Persistence is not included in this example.

For a larger app, move `Task` and business operations into a model/service class,
keep view composition in the module, and extract repeated UI into functional
components. Register additional modules for separate screens; use tabs for
related views inside one screen.

Optional renderers can be enabled independently through the existing CMake
options: `UIUTILS_ENABLE_CODES`, `UIUTILS_ENABLE_CHARTS`, and
`UIUTILS_ENABLE_PDF`. They are unrelated to the Todo app's core controls.

## Validation and limits

The exact Todo source above passed a C++17 syntax check against uiutils 0.7.0's
public headers. Backend compilation, linking, and GUI execution were not tested
in the tutorial environment because CMake and wxWidgets were unavailable.
The numbered checks above are a manual verification guide, not reported test
results. The app has no persistence, background jobs, or automatic recomposition.
