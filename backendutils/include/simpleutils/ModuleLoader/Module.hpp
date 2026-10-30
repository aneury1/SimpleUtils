#pragma once

#include <string>

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <map>

class Module
{
  public:
    Module(const std::string& module_name) : module_name(module_name)
    {
    }
    ~Module(){};

    const std::string& GetModuleName() const
    {
        return module_name;
    }

  private:
    std::string module_name;
};

using ModuleMap = std::map<std::string, std::shared_ptr<Module>>;
