#pragma once

#include "simpleutils/ModuleLoader/ModuleLoader.hpp"

class ModuleLoader;

template <typename GUI>
class ApplicationContext {
 public:
  ApplicationContext(GUI& gui, ModuleLoader& moduleLoader)
      : _gui(gui), _moduleLoader(moduleLoader) {}

  ~ApplicationContext() {}

  GUI& getGui() { return _gui; }

  ModuleLoader& getModuleLoader() const { return _moduleLoader; }

 private:
  GUI& _gui;
  ModuleLoader& _moduleLoader;
};