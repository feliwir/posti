#include "interpreter.hpp"
#include "parser.hpp"
#include "builtins.hpp"
#include <string>

ps::Interpreter::Interpreter()
{
  m_systemDict = Builtins::CreateDictionary(this);
}

void ps::Interpreter::Load(std::istream& input)
{
  Parser parser(input);

  while (auto obj = parser.GetObject())
  {
    //Push literal objects to the operand stack
    if (!obj->IsExecutable())
      m_opStack.push(obj);
    else
    {
      //do some check stuff
    }
  }
}
