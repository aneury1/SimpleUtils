##### Build
install your project, 

##### usage 

```cpp
find_package(uiutils CONFIG REQUIRED)

add_executable(myapp main.cpp)
target_link_libraries(myapp PRIVATE uiutils::uiutils simpleutils::simpleutils)
```