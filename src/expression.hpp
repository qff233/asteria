// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_EXPRESSION_HPP_
#define ASTERIA_EXPRESSION_HPP_

#include "fwd.hpp"
#include "type_tuple.hpp"
#include <type_traits> // std::enable_if, std::decay, std::is_base_of

namespace Asteria {

class Expression {
public:
	enum Category : unsigned {
		category_subexpression                = 0,
		category_prefix_expression            = 1,
		category_lambda_expression            = 2,
		category_infix_or_postfix_expression  = 3,
	};

	using Types = Type_tuple< Expression_list        // 0
	                        , Key_value_list         // 1
	                        , Value_ptr<Expression>  // 2
		>;

private:
	Types::rebound_variant m_variant;

public:
	Expression()
		: m_variant()
	{ }
	template<typename ValueT, typename std::enable_if<std::is_base_of<Variable, typename std::decay<ValueT>::type>::value == false>::type * = nullptr>
	Expression(ValueT &&value)
		: m_variant(std::forward<ValueT>(value))
	{ }
	~Expression();

public:
	Category get_category() const noexcept {
		return static_cast<Category>(m_variant.which());
	}

	Value_ptr<Variable> evaluate() const;
};

}

#endif
