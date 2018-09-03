#include "interpreter.hpp"
#include "parser.hpp"
#include <string>

void ps::Interpreter::Load(std::istream& input)
{
  Parser parser(input);

  while (auto obj = parser.GetObject())
  {
    //Push literal objects to the operand stack
    if (!obj->IsExecutable())
      m_operands.push(obj);
    else
    {
      //do some check stuff
    }
  }
}
