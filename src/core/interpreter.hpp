#pragma once
#include <istream>
#include <stack>
#include <map>

namespace ps
{
  class Object;

  class Interpreter
  {
  public:
    void Load(std::istream& input);

  private:
    std::stack<std::shared_ptr<Object>> m_operands;
    std::stack<std::map<std::string, std::shared_ptr<Object>> > m_dictionary;
  };
}