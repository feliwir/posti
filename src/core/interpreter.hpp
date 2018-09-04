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
    Interpreter();
    void Load(std::istream& input);

    inline std::stack<std::shared_ptr<Object>>& GetOperandStack()
    {
      return m_opStack;
    }

  private:
    std::stack<std::shared_ptr<Object>> m_opStack;
    std::stack<std::map<std::string, std::shared_ptr<Object>> > m_dictStack;
    std::map < std::string, std::shared_ptr<Object>>  m_systemDict;
  };
}