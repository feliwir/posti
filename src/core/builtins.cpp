#include "builtins.hpp"
#include "interpreter.hpp"

void ps::Builtins::CreateOperand(std::string_view name, std::function<void()> func)
{
	m_dict[std::string(name)] = std::make_shared<OperandObject>(func);
}

struct abs {
	template <typename T>
	auto operator()(T&& i)
		-> decltype(i) {
		return std::abs(i);
	}
};


std::map<std::string, std::shared_ptr<ps::Object>>& ps::Builtins::CreateDictionary(Interpreter* interpr)
{
	m_interpr = interpr;

	//STACK
	//POP
	CreateOperand("pop", [this]() {
		Pop();
		});

	//EXCH
	CreateOperand("exch", [this]() {
		auto first = Pop();
		auto second = Pop();
		Push(first);
		Push(second);
		});

	//DUP
	CreateOperand("dup", [this]() {
		Push(Top());
		});

	//COPY
	CreateOperand("copy", [&]() {
		int n = Pop<int>();
		auto vec = Pop(n);
		std::reverse(vec.begin(), vec.end());
		Push(vec);
		Push(vec);
		});

	// //index
	// dict["index"] = std::make_shared<OperandObject>([&]() {
	//   int n = popInt();
	//   auto vec = std::vector<std::shared_ptr<ps::Object>>();
	//   for (int i = 0; i < n + 1; i++)
	//   	vec.push_back(pop());
	//   for (int i = n; i >= 0; i--)
	//   	s.push(vec[i]);
	//   s.push(vec[n]);
	// });

	//clear
	CreateOperand("clear", [this]() {
		auto s = GetStack();
		while (!s.empty())
			s.pop();
		});

	//count
	CreateOperand("count", [this]() {
		Push<int>(GetStack().size());
		});

	//ARITHMETIC
	//ADD
	CreateOperand("add", [this]() {
		BinaryOp(std::plus<>{});
		});

	//DIV
	CreateOperand("div", [this]() {
		BinaryOp(std::divides<>{});
		});

	//IDIV
	CreateOperand("idiv", [this]() {
		BinaryOp(std::divides<int>{});
		});

	//MOD
	CreateOperand("mod", [this]() {
		BinaryOp(std::modulus<>{});
		});

	//MUL
	CreateOperand("mul", [this]() {
		BinaryOp(std::multiplies<>{});
		});

	//SUB
	CreateOperand("sub", [this]() {
		BinaryOp(std::minus<>{});
		});

	//ABS
	//CreateOperand("abs", [this]() {
	//	UnaryOp(Builtins::abs<>{});
	//	});

	//NEG
	CreateOperand("neg", [this]() {
		UnaryOp(std::negate<>{});
		});

	/*
	  //CEILING
	  CreateOperand("ceiling", [this]() {
		UnaryOp(std::ceil<>);
	  });
	*/

	return m_dict;
}

std::stack<std::shared_ptr<ps::Object>>& ps::Builtins::GetStack()
{
	return m_interpr->GetOperandStack();
}