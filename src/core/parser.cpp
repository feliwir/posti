#include "parser.hpp"
#include "util.hpp"
#include "objects/integer.hpp"
#include "objects/real.hpp"
#include "objects/name.hpp"
#include <string>
#include <cctype>

ps::Parser::Parser(std::istream &input) : m_input(input)
{
}

std::shared_ptr<ps::Object> ps::Parser::GetObject()
{
  m_buffer.clear();
  char c = 0;
  Mode m = Mode::None;
  bool finished = false;
  std::shared_ptr<Object> result;

  while (!finished && m_input.get(c))
  {
    //just skip any of these
    if ((c == '\r') || (c== '\t'))
      continue;

    //Skip any comments
    if (m == Mode::Comment)
    {
      if (isNewline(c))
        m = Mode::None;

      continue;
    }

    //Handle delimitters
    if (isWhitespace(c) || isNewline(c))
    {
      if (m != Mode::None)
        finished = true;

      continue;
    }

    //Go into comment mode
    if (m != Mode::String && isComment(c))
    {
      m = Mode::Comment;
      continue;
    }

    //Store the character
    m_buffer += c;

    //Handle any transitions
    switch (m)
    {
    case Mode::None:
      if (std::isdigit(c) || isSign(c))
        m = Mode::Integer;
      else
        m = Mode::Name;
      break;
    case Mode::Integer:
      if (c == 'E' || c == '.')
        m = Mode::Real;
      else if (!std::isdigit(c))
        m = Mode::Name;
      break;
    case Mode::Real:
      if (c == 'E' || c == '.')
        m = Mode::Name;
      break;
    case Mode::String:
      break;
    case Mode::Array:
      break;
    default:
      break;
    }
  }

  switch (m)
  {
  case Mode::Name:
    result = std::make_shared<NameObject>(m_buffer);
    break;
  case Mode::Integer:
    result = std::make_shared<IntegerObject>(std::stoi(m_buffer));
    break;
  case Mode::Real:
    result = std::make_shared<RealObject>(std::stof(m_buffer));
    break;
  }

  return result;
}