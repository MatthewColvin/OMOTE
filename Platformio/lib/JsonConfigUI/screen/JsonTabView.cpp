#include "JsonTabView.hpp"

#include "HardwareFactory.hpp"
#include "JsonPage.hpp"

using namespace UI::Page;

JsonTabView::JsonTabView(std::string aFileName)
    : TabView(ID(ID::Pages::HomeScreenTabView)) {
  File fp = HardwareFactory::getAbstract().littleFs()->open(aFileName, LFS_O_RDONLY);
  if (!fp)
    return;
  std::string content = fp.read(10000);

  MemConsciousDocument d;
  d.Parse(content.c_str());

  if (d.HasMember("Pages")) {
    for (rapidjson::SizeType i = 0; i < d["Pages"].Size(); i++) {
      if (d["Pages"][i].HasMember("FileName")) {
        std::string fileName = d["Pages"][i]["FileName"].GetString();
        std::string pageName;
        if (d["Pages"][i].HasMember("PageName"))
          pageName = d["Pages"][i]["PageName"].GetString();
        else
          pageName = fileName;
        std::string commandPrefix;
        if (d["Pages"][i].HasMember("CommandPrefix"))
          commandPrefix = d["Pages"][i]["CommandPrefix"].GetString();

        AddTab(std::make_unique<UI::Page::JsonPage>(fileName, pageName, commandPrefix));
      }
    }
  }
}
