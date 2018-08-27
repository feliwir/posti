#pragma once
#include <string_view>

namespace ps
{
  class Parser
  {
  public:
    void Load(const std::string_view& view);
  };
}