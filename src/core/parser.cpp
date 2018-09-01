#include "parser.hpp"
#include "util.hpp"
#include <string>

ps::Parser::Parser(std::istream& input) : m_input(input)
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

std::shared_ptr<ps::Object> ps::Parser::GetObject() const
{
  return nullptr;
}