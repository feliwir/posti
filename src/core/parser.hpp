#pragma once
#include <istream>
#include <vector>
#include <memory>
#include "object.hpp"

namespace ps
{

class Parser
{
public:
  enum Error
  {
    LimitCheck = 0,
  };

  Parser(std::istream &input);

  std::shared_ptr<Object> GetObject();

private:
  enum class Mode
  {
    None,
    Integer,
    String,
    Real,
    Name,
    Array,
    Comment,
    Error
  };

  inline bool isComment(const char c) const
  {
    return c == '%';
  }

  inline bool isNewline(const char c) const
  {
    return c == '\n';
  }

  inline bool isWhitespace(const char c) const
  {
    return c == ' ';
  }

  inline bool isSign(const char c) const
  {
    return c == '+' || c == '-';
  }

private:
  std::istream &m_input;
  std::string m_buffer;
  Error m_err;
};
} // namespace ps