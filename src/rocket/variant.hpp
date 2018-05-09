// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_VARIANT_HPP_
#define ROCKET_VARIANT_HPP_

#include <cstddef> // std::size_t
#include <type_traits> // so many
#include <stdexcept> // std::invalid_argument
#include <utility> // std::move(), std::forward(), std::declval(), std::swap()
#include <new> // placement new
#include "assert.hpp"

namespace rocket {

using ::std::size_t;
using ::std::is_convertible;
using ::std::decay;
using ::std::enable_if;
using ::std::is_base_of;
using ::std::remove_cv;
using ::std::is_nothrow_constructible;
using ::std::is_nothrow_assignable;
using ::std::is_nothrow_copy_constructible;
using ::std::is_nothrow_move_constructible;
using ::std::is_nothrow_copy_assignable;
using ::std::is_nothrow_move_assignable;
using ::std::is_nothrow_destructible;
using ::std::invalid_argument;

template<typename ...elementsT>
class variant;

namespace details_variant {
	template<size_t indexT, typename ...typesT>
	struct type_getter {
		static_assert(indexT && false, "`indexT` was out of range.");
	};
	template<typename firstT, typename ...remainingT>
	struct type_getter<0, firstT, remainingT...> {
		using type = firstT;
	};
	template<size_t indexT, typename firstT, typename ...remainingT>
	struct type_getter<indexT, firstT, remainingT...> {
		using type = typename type_getter<indexT - 1, remainingT...>::type;
	};

	template<size_t indexT, typename expectT, typename ...typesT>
	struct type_finder {
		static_assert(indexT && false, "`expectT` could not be found.");
	};
	template<size_t indexT, typename expectT, typename firstT, typename ...remainingT>
	struct type_finder<indexT, expectT, firstT, remainingT...> {
		enum : size_t { value = type_finder<indexT + 1, expectT, remainingT...>::value };
	};
	template<size_t indexT, typename expectT, typename ...remainingT>
	struct type_finder<indexT, expectT, expectT, remainingT...> {
		enum : size_t { value = indexT };
	};

	template<typename expectT, typename ...typesT>
	struct has_type_recursive {
		enum : bool { value = false };
	};
	template<typename expectT, typename firstT, typename ...remainingT>
	struct has_type_recursive<expectT, firstT, remainingT...> {
		enum : bool { value = has_type_recursive<expectT, remainingT...>::value };
	};
	template<typename expectT, typename ...remainingT>
	struct has_type_recursive<expectT, expectT, remainingT...> {
		enum : bool { value = true };
	};
	template<typename expectT, typename ...elementsT, typename ...remainingT>
	struct has_type_recursive<expectT, variant<elementsT...>, remainingT...> {
		enum : bool { value = has_type_recursive<expectT, elementsT...>::value || has_type_recursive<expectT, remainingT...>::value };
	};
	template<typename ...elementsT, typename ...remainingT>
	struct has_type_recursive<variant<elementsT...>, variant<elementsT...>, remainingT...> {
		enum : bool { value = true };
	};

	template<bool conditionT, size_t trueT, typename falseT>
	struct conditional_index {
		enum : size_t { value = trueT };
	};
	template<size_t trueT, typename falseT>
	struct conditional_index<false, trueT, falseT> {
		enum : size_t { value = falseT::value };
	};

	template<size_t indexT, typename expectT, typename ...typesT>
	struct recursive_type_finder {
		static_assert(indexT && false, "`expectT` could not be found.");
	};
	template<size_t indexT, typename expectT, typename firstT, typename ...remainingT>
	struct recursive_type_finder<indexT, expectT, firstT, remainingT...> {
		enum : size_t { value = conditional_index<has_type_recursive<expectT, firstT>::value, indexT, recursive_type_finder<indexT + 1, expectT, remainingT...>>::value };
	};

	template<typename ...typesT>
	struct conjunction {
		enum : bool { value = true };
	};
	template<typename firstT, typename ...remainingT>
	struct conjunction<firstT, remainingT...> {
		enum : bool { value = firstT::value ? conjunction<remainingT...>::value : false };
	};

	template<size_t indexT, typename elementT>
	struct storage_for {
		alignas(elementT) char data[sizeof(elementT)];

		const elementT * get() const {
			const auto ptr = static_cast<const void *>(this->data);
			return static_cast<const elementT *>(ptr);
		}
		elementT * get(){
			const auto ptr = static_cast<void *>(this->data);
			return static_cast<elementT *>(ptr);
		}
	};

	template<size_t indexT, typename ...elementsT>
	struct variant_buffer {
		template<typename visitorT>
		void apply_visitor(size_t /*expect*/, visitorT &&/*visitor*/) const {
			ROCKET_ASSERT_MSG(false, "type index expected is out of range");
		}
		template<typename visitorT>
		void apply_visitor(size_t /*expect*/, visitorT &&/*visitor*/){
			ROCKET_ASSERT_MSG(false, "type index expected is out of range");
		}
	};

	template<size_t indexT, typename firstT, typename ...remainingT>
	struct variant_buffer<indexT, firstT, remainingT...> : private storage_for<indexT, firstT>, public variant_buffer<indexT + 1, remainingT...> {
		template<typename visitorT>
		void apply_visitor(size_t expect, visitorT &&visitor) const {
			if(expect == indexT){
				::std::forward<visitorT>(visitor).template dispatch<indexT>(this->storage_for<indexT, firstT>::get());
			} else {
				this->variant_buffer<indexT + 1, remainingT...>::apply_visitor(expect, ::std::forward<visitorT>(visitor));
			}
		}
		template<typename visitorT>
		void apply_visitor(size_t expect, visitorT &&visitor){
			if(expect == indexT){
				::std::forward<visitorT>(visitor).template dispatch<indexT>(this->storage_for<indexT, firstT>::get());
			} else {
				this->variant_buffer<indexT + 1, remainingT...>::apply_visitor(expect, ::std::forward<visitorT>(visitor));
			}
		}
	};

	template<typename expectT>
	struct visitor_get_pointer {
		expectT *result_ptr;

		template<size_t indexT, typename elementT>
		void dispatch(elementT *ptr){
			this->result_ptr = ptr;
		}
	};

	struct visitor_value_initialize {
		template<size_t indexT, typename elementT>
		void dispatch(elementT *ptr){
			::new(static_cast<void *>(ptr)) elementT();
		}
	};

	struct visitor_copy_construct_from {
		const void *source_ptr;

		template<size_t indexT, typename elementT>
		void dispatch(elementT *ptr){
			::new(static_cast<void *>(ptr)) elementT(*static_cast<const elementT *>(this->source_ptr));
		}
	};

	struct visitor_move_construct_from {
		void *source_ptr;

		template<size_t indexT, typename elementT>
		void dispatch(elementT *ptr){
			::new(static_cast<void *>(ptr)) elementT(::std::move(*static_cast<elementT *>(this->source_ptr)));
		}
	};

	struct visitor_copy_assign_from {
		const void *source_ptr;

		template<size_t indexT, typename elementT>
		void dispatch(elementT *ptr){
			*ptr = *static_cast<const elementT *>(this->source_ptr);
		}
	};

	struct visitor_move_assign_from {
		void *source_ptr;

		template<size_t indexT, typename elementT>
		void dispatch(elementT *ptr){
			*ptr = ::std::move(*static_cast<elementT *>(this->source_ptr));
		}
	};

	struct visitor_swap {
		void *source_ptr;

		template<size_t indexT, typename elementT>
		void dispatch(elementT *ptr){
			using ::std::swap;
			swap(*ptr, *static_cast<elementT *>(this->source_ptr));
		}
	};

	struct visitor_destroy {
		template<size_t indexT, typename elementT>
		void dispatch(elementT *ptr){
			ptr->~elementT();
		}
	};

	template<typename fvisitorT>
	struct visitor_forward {
		fvisitorT &fvisitor;

		template<size_t indexT, typename elementT>
		void dispatch(elementT *ptr){
			::std::forward<fvisitorT>(this->fvisitor)(*ptr);
		}
	};

	template<typename paramT>
	struct is_nothrow_forward_constructible {
		enum : bool { value = is_nothrow_constructible<typename decay<paramT>::type, paramT &&>::value };
	};
	template<typename paramT>
	struct is_nothrow_forward_assignable {
		enum : bool { value = is_nothrow_assignable<typename decay<paramT>::type, paramT &&>::value };
	};

	namespace nothrow_swappable_helper {
		using ::std::swap;

		template<typename paramT>
		void check() noexcept(noexcept(swap(::std::declval<paramT &>(), ::std::declval<paramT &>())));
	}

	template<typename paramT>
	struct is_nothrow_swappable {
		enum : bool { value = noexcept(nothrow_swappable_helper::check<paramT>()) };
	};
}

template<typename ...elementsT>
class variant {
public:
	static_assert(details_variant::conjunction<is_nothrow_destructible<elementsT>...>::value, "Destructors of candidates are not allowed to throw exceptions.");

	template<size_t indexT>
	struct at {
		using type = typename details_variant::type_getter<indexT, elementsT...>::type;
	};
	template<typename expectT>
	struct index_of {
		enum : size_t { value = details_variant::type_finder<0, expectT, elementsT...>::value };
	};

	template<typename ...addT>
	struct prepend {
		using type = variant<addT..., elementsT...>;
	};
	template<typename ...addT>
	struct append {
		using type = variant<elementsT..., addT...>;
	};

private:
	details_variant::variant_buffer<0, elementsT...> m_buffer;
	size_t m_index;

public:
	variant() noexcept(is_nothrow_constructible<typename details_variant::type_getter<0, elementsT...>::type>::value) {
		details_variant::visitor_value_initialize visitor = { };
		this->m_buffer.apply_visitor(0, visitor);
		this->m_index = 0;
	}
	template<typename elementT, typename enable_if<is_base_of<variant, typename decay<elementT>::type>::value == false>::type * = nullptr>
	variant(elementT &&element) noexcept(details_variant::is_nothrow_forward_constructible<elementT>::value) {
		enum : size_t { eindex = details_variant::recursive_type_finder<0, typename decay<elementT>::type, elementsT...>::value };
		using etype = typename details_variant::type_getter<eindex, elementsT...>::type;
		details_variant::visitor_get_pointer<void> visitor = { nullptr };
		this->m_buffer.apply_visitor(eindex, visitor);
		new(visitor.result_ptr) etype(::std::forward<elementT>(element));
		this->m_index = eindex;
	}
	variant(const variant &rhs) noexcept(details_variant::conjunction<is_nothrow_copy_constructible<elementsT>...>::value) {
		details_variant::visitor_get_pointer<const void> visitor_rhs = { nullptr };
		rhs.m_buffer.apply_visitor(rhs.m_index, visitor_rhs);
		details_variant::visitor_copy_construct_from visitor = { visitor_rhs.result_ptr };
		this->m_buffer.apply_visitor(rhs.m_index, visitor);
		this->m_index = rhs.m_index;
	}
	variant(variant &&rhs) noexcept(details_variant::conjunction<is_nothrow_move_constructible<elementsT>...>::value) {
		details_variant::visitor_get_pointer<void> visitor_rhs = { nullptr };
		rhs.m_buffer.apply_visitor(rhs.m_index, visitor_rhs);
		details_variant::visitor_move_construct_from visitor = { visitor_rhs.result_ptr };
		this->m_buffer.apply_visitor(rhs.m_index, visitor);
		this->m_index = rhs.m_index;
	}
	template<typename elementT, typename enable_if<is_base_of<variant, typename decay<elementT>::type>::value == false>::type * = nullptr>
	variant & operator=(elementT &&element) noexcept(details_variant::is_nothrow_forward_assignable<elementT>::value && details_variant::is_nothrow_forward_constructible<elementT>::value) {
		enum : size_t { eindex = details_variant::recursive_type_finder<0, typename decay<elementT>::type, elementsT...>::value };
		using etype = typename details_variant::type_getter<eindex, elementsT...>::type;
		if(this->m_index == eindex){
			details_variant::visitor_get_pointer<void> visitor = { nullptr };
			this->m_buffer.apply_visitor(eindex, visitor);
			*static_cast<etype *>(visitor.result_ptr) = ::std::forward<elementT>(element);
		} else {
			details_variant::visitor_get_pointer<void> visitor = { nullptr };
			this->m_buffer.apply_visitor(eindex, visitor);
			new(visitor.result_ptr) etype(::std::forward<elementT>(element));
			details_variant::visitor_destroy cleaner = { };
			this->m_buffer.apply_visitor(this->m_index, cleaner);
		}
		this->m_index = eindex;
		return *this;
	}
	variant & operator=(const variant &rhs) noexcept(details_variant::conjunction<is_nothrow_copy_assignable<elementsT>..., is_nothrow_copy_constructible<elementsT>...>::value) {
		details_variant::visitor_get_pointer<const void> visitor_rhs = { nullptr };
		rhs.m_buffer.apply_visitor(rhs.m_index, visitor_rhs);
		if(this->m_index == rhs.m_index){
			details_variant::visitor_copy_assign_from visitor = { visitor_rhs.result_ptr };
			this->m_buffer.apply_visitor(rhs.m_index, visitor);
		} else {
			details_variant::visitor_copy_construct_from visitor = { visitor_rhs.result_ptr };
			this->m_buffer.apply_visitor(rhs.m_index, visitor);
			details_variant::visitor_destroy cleaner = { };
			this->m_buffer.apply_visitor(this->m_index, cleaner);
		}
		this->m_index = rhs.m_index;
		return *this;
	}
	variant & operator=(variant &&rhs) noexcept(details_variant::conjunction<is_nothrow_move_assignable<elementsT>..., is_nothrow_move_constructible<elementsT>...>::value) {
		details_variant::visitor_get_pointer<void> visitor_rhs = { nullptr };
		rhs.m_buffer.apply_visitor(rhs.m_index, visitor_rhs);
		if(this->m_index == rhs.m_index){
			details_variant::visitor_move_assign_from visitor = { visitor_rhs.result_ptr };
			this->m_buffer.apply_visitor(rhs.m_index, visitor);
		} else {
			details_variant::visitor_move_construct_from visitor = { visitor_rhs.result_ptr };
			this->m_buffer.apply_visitor(rhs.m_index, visitor);
			details_variant::visitor_destroy cleaner = { };
			this->m_buffer.apply_visitor(this->m_index, cleaner);
		}
		this->m_index = rhs.m_index;
		return *this;
	}
	~variant(){
		details_variant::visitor_destroy cleaner = { };
		this->m_buffer.apply_visitor(this->m_index, cleaner);
	}

public:
	size_t index() const noexcept {
		return this->m_index;
	}
	template<typename elementT>
	const elementT * try_get() const noexcept {
		enum : size_t { eindex = index_of<typename remove_cv<elementT>::type>::value };
		if(this->m_index != eindex){
			return nullptr;
		}
		details_variant::visitor_get_pointer<const void> visitor = { nullptr };
		this->m_buffer.apply_visitor(eindex, visitor);
		return static_cast<const elementT *>(visitor.result_ptr);
	}
	template<typename elementT>
	elementT * try_get() noexcept {
		enum : size_t { eindex = index_of<typename remove_cv<elementT>::type>::value };
		if(this->m_index != eindex){
			return nullptr;
		}
		details_variant::visitor_get_pointer<void> visitor = { nullptr };
		this->m_buffer.apply_visitor(eindex, visitor);
		return static_cast<elementT *>(visitor.result_ptr);
	}
	template<typename elementT>
	const elementT & get() const {
		const auto ptr = this->try_get<elementT>();
		if(!ptr){
			throw invalid_argument("variant::get(): `elementT` does not match the type of the element that is currently active.");
		}
		return *ptr;
	}
	template<typename elementT>
	elementT & get(){
		const auto ptr = this->try_get<elementT>();
		if(!ptr){
			throw invalid_argument("variant::get(): `elementT` does not match the type of the element that is currently active.");
		}
		return *ptr;
	}
	template<typename elementT>
	void set(elementT &&element) noexcept(noexcept(::std::declval<variant &>() = ::std::forward<elementT>(element))) {
		*this = ::std::forward<elementT>(element);
	}

	template<typename fvisitorT>
	void visit(fvisitorT &&fvisitor) const {
		details_variant::visitor_forward<fvisitorT> visitor = { fvisitor };
		this->m_buffer.apply_visitor(this->m_index, visitor);
	}
	template<typename fvisitorT>
	void visit(fvisitorT &&fvisitor){
		details_variant::visitor_forward<fvisitorT> visitor = { fvisitor };
		this->m_buffer.apply_visitor(this->m_index, visitor);
	}

	void swap(variant &rhs) noexcept(details_variant::conjunction<details_variant::is_nothrow_swappable<elementsT>..., is_nothrow_move_constructible<elementsT>...>::value) {
		details_variant::visitor_get_pointer<void> visitor_rhs = { nullptr };
		rhs.m_buffer.apply_visitor(rhs.m_index, visitor_rhs);
		if(this->m_index == rhs.m_index){
			details_variant::visitor_swap visitor = { visitor_rhs.result_ptr };
			this->m_buffer.apply_visitor(rhs.m_index, visitor);
		} else {
			details_variant::visitor_move_construct_from visitor_s1 = { visitor_rhs.result_ptr };
			this->m_buffer.apply_visitor(rhs.m_index, visitor_s1);
			try {
				details_variant::visitor_get_pointer<void> visitor_lhs = { nullptr };
				this->m_buffer.apply_visitor(this->m_index, visitor_lhs);
				details_variant::visitor_move_construct_from visitor_s2 = { visitor_lhs.result_ptr };
				rhs.m_buffer.apply_visitor(this->m_index, visitor_s2);
			} catch(...){
				details_variant::visitor_destroy cleaner = { };
				this->m_buffer.apply_visitor(rhs.m_index, cleaner);
				noexcept(swap(rhs)) ? static_cast<void>(0) : throw;
			}
			details_variant::visitor_destroy cleaner = { };
			this->m_buffer.apply_visitor(this->m_index, cleaner);
			rhs.m_buffer.apply_visitor(rhs.m_index, cleaner);
		}
		::std::swap(this->m_index, rhs.m_index);
	}
};

template<typename ...elementsT>
void swap(variant<elementsT...> &lhs, variant<elementsT...> &rhs) noexcept(noexcept(lhs.swap(rhs))) {
	lhs.swap(rhs);
}

}

#endif
