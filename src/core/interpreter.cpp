#include "interpreter.hpp"
#include "parser.hpp"
#include <string>

void ps::Interpreter::Load(std::istream& input)
{
  Parser parser(input);

  while (parser.GetObject())
  {

  }
}
