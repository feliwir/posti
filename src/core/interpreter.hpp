#pragma once
#include <istream>
#include <stack>
#include <deque>
#include <map>
#include <memory>

namespace ps
{
class Object;
enum class ScriptMode
{
  Standalone,
  Embedded,
};

class Interpreter
{
public:
  Interpreter(ScriptMode mode = ScriptMode::Standalone);
  void Load(std::istream &input);

  inline std::stack<std::shared_ptr<Object>> &GetOperandStack()
  {
    return m_opStack;
  }

private:
  std::shared_ptr<Object> DictLookup(std::shared_ptr<Object> name);

private:
  std::stack<std::shared_ptr<Object>> m_opStack;
  std::deque<std::map<std::string, std::shared_ptr<Object>>> m_dictStack;
  std::map<std::string, std::shared_ptr<Object>> m_systemDict;
  ScriptMode m_mode;
};
} // namespace ps