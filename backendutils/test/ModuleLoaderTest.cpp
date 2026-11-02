#include <gtest/gtest.h>

#include <string>

#include "simpleutils/ModuleLoader/Module.hpp"
#include "simpleutils/ModuleLoader/ModuleLoader.hpp"

// Mock Module class for testing
class MockModule : public Module {
 public:
  MockModule(const std::string& name) : Module(name) {}
  const std::string& GetModuleName() const { return Module::GetModuleName(); }
};

// Derived class to implement virtual functions for testing
class TestModuleLoader : public ModuleLoader {
 public:
  TestModuleLoader(const std::string& extension) : ModuleLoader(extension) {}

  bool Load(const std::string& module_name, void* parent) override {
    loadedModules[module_name] = std::make_shared<MockModule>(module_name);
    return true;
  }

  bool Unload(const std::string& module_name, void* parent) override {
    loadedModules.erase(module_name);
    return true;
  }

  const std::string GetFilename(const std::string& absolute_path) override {
    return absolute_path.substr(absolute_path.find_last_of('/') + 1);
  }
};

TEST(ModuleLoaderTest, ConstructorAndExtension) {
  TestModuleLoader loader(".so");
  EXPECT_EQ(loader.GetExtension(), ".so");
}

TEST(ModuleLoaderTest, LoadUnloadsModule) {
  TestModuleLoader loader(".so");
  loader.Load("test_module", nullptr);
  EXPECT_TRUE(loader.GetLoadedModules().count("test_module") > 0);

  loader.Unload("test_module", nullptr);
  EXPECT_TRUE(loader.GetLoadedModules().count("test_module") == 0);
}

TEST(ModuleLoaderTest, GetFilename) {
  TestModuleLoader loader(".so");
  std::string absolute_path = "/home/user/modules/test_module.so";
  EXPECT_EQ(loader.GetFilename(absolute_path), "test_module.so");
}

TEST(ModuleLoaderTest, GetExtension) {
  TestModuleLoader loader(".so");
  EXPECT_EQ(loader.GetExtension(), ".so");
}