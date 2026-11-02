#pragma once

#include "ApplicationContext.hpp"
#include "ModuleLoader/Module.hpp"
#include <memory>

#define INIT_FUNC_NAME "OnInitPlugin"
#define DEINIT_FUNC_NAME "OnDeinitPlugin"

class ModuleLoader
{
  public:
    ModuleLoader(const std::string& extension) : fileExtension(extension)
    {
    }

    // virtual bool Load(const std::string& module_name, wxObject* parent) = 0;
    virtual bool Load(const std::string& module_name,
                      ApplicationContext<wxObject>&& parent) = 0;
    virtual bool Unload(const std::string& module_name, wxObject* parent) = 0;
    virtual const std::string GetFilename(const std::string& absolute_path) = 0;
    const std::string& GetExtension() const
    {
        return fileExtension;
    }
    const ModuleMap& GetLoadedModules() const
    {
        return loadedModules;
    }

  protected:
    ModuleMap loadedModules;
    std::string fileExtension;
};