#pragma once
#include <string_view>

namespace ps
{
class util
{
public:
  inline static std::string_view TrimFront(std::string_view view)
  {
    int first = view.find_first_not_of(' ');

    if (first != view.npos)
      return view.substr(first, view.size() - first);
    else
      return view;
  }
};
} // namespace ps