#include "parser.hpp"
#include "util.hpp"
#include <string>
#include <cctype>

ps::Parser::Parser(std::istream& input) : m_input(input)
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
    if (c == '\r')
      continue;

    //TODO: simplify this a lot
    switch (m)
    {
    case ps::Parser::Mode::None:
      if (isWhitespace(c) || isNewline(c))
      {
        continue;
      }
      else if (isComment(c))
      {
        m = Mode::Comment;
      }
      else if (std::isdigit(c) || isSign(c) )
      {
        m_buffer += c;
        m = Mode::Integer;
      }
      else
      {
        m_buffer += c;
        m = Mode::Name;
      }
      break;
    case ps::Parser::Mode::Integer:
      if (isWhitespace(c) || isNewline(c))
      {
        finished = true;
      }
      else if (std::isdigit(c))
      {
        m_buffer += c;
      }
      else if (c == 'E' || c == '.')
      {
        m_buffer += c;
        m = Mode::Real;
      }
      else
      {
        m_buffer += c;
        m = Mode::Name;
      }
      break;
    case ps::Parser::Mode::Real:
      if (isWhitespace(c) || isNewline(c))
      {
        finished = true;
      }
      else if (std::isdigit(c))
      {
        m_buffer += c;
      }
      else if (c == 'E' || c == '.')
      {
        m_buffer += c;
        m = Mode::Name;
      }
      break;
    case ps::Parser::Mode::String:
      break;
    case ps::Parser::Mode::Name:
      if (isWhitespace(c) || isNewline(c))
      {
        finished = true;
      }
      else
      {
        m_buffer += c;
      }
      break;
    case ps::Parser::Mode::Array:
      break;
    case ps::Parser::Mode::Comment:
      if (isNewline(c))
      {
        m = Mode::None;
      }
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