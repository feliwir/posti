#include <gtest/gtest.h>
#include "interpreter.hpp"

TEST(Interpreter, Arithmetic)
{
	std::string content = "100 2 200 100 200 add sub mul div";
	std::stringstream input(content);

	ps::Interpreter psi;
	EXPECT_TRUE(psi.Load(input));

	const auto& stack = psi.GetOperandStack();
	ASSERT_EQ(stack.size(), 1) << "Stack size should have been 1!";

	auto& object = stack.top();
	ASSERT_NE(object, nullptr) << "Object shouldn't be null!";

	EXPECT_EQ(object->GetAccess(), ps::ObjectAccess::Unlimited) << "Expected access flag 'Unlimited'!";

	EXPECT_EQ(object->GetType(), ps::ObjectType::Integer) << "Expected integer!";

	auto integer = std::static_pointer_cast<ps::IntegerObject>(object);

	ASSERT_NE(integer, nullptr) << "Object is not integer!";

	EXPECT_EQ(integer->GetValue(), 2) << "Result isn't 2!";
}

TEST(Interpreter, Path)
{
	std::string content = R"(
	newpath
	100 200 moveto
	200 250 lineto
	100 300 lineto
	closepath
	gsave
	0.5 setgray
	fill
	grestore
	4 setlinewidth
	0.75 setgray
	stroke)";

	std::stringstream input(content);
	ps::Interpreter psi;
	// Missing "newpath"
	EXPECT_FALSE(psi.Load(input));
}