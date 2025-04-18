#pragma once
#include <string>

#include "TabView.hpp"

namespace UI::Page {

class JsonTabView : public TabView {
public:
  JsonTabView(std::string aFileName);
};

} // namespace UI::Page