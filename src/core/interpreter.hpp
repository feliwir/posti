#pragma once
#include <istream>
#include <stack>
#include <deque>
#include <map>
#include <memory>

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
    std::shared_ptr<Object> DictLookup(std::shared_ptr<Object> name);

  private:
    std::stack<std::shared_ptr<Object>> m_opStack;
    std::deque<std::map<std::string, std::shared_ptr<Object>> > m_dictStack;
    std::map < std::string, std::shared_ptr<Object>>  m_systemDict;
  };
}