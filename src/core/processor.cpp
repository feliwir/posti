#include "processor.hpp"
#include "util.hpp"
#include <string>

void ps::Processor::Load(std::istream& input)
{
  std::string line;

  while (std::getline(input, line))
  {
    auto lineView = util::TrimFront(line);

    //Skip empty lines
    if (lineView.empty())
      continue;

    //Skip comments 
    //TODO: don't skip meta data or version tag
    if (lineView.front() == '%')
      continue;


  }
}
