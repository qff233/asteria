// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_TRACEABLE_EXCEPTION_HPP_
#define ASTERIA_RUNTIME_TRACEABLE_EXCEPTION_HPP_

#include "../fwd.hpp"
#include "value.hpp"
#include "../syntax/source_location.hpp"

namespace Asteria {

class Traceable_Exception : public virtual std::exception
  {
  private:
    Source_Location m_loc;
    Value m_value;
    Cow_Vector<Source_Location> m_backtrace;

  public:
    template<typename XvalueT,ROCKET_ENABLE_IF(std::is_constructible<Value, XvalueT &&>::value)>
      Traceable_Exception(const Source_Location &loc, XvalueT &&value)
      : m_loc(loc), m_value(std::forward<XvalueT>(value))
      {
      }
    explicit Traceable_Exception(const std::exception &stdex)
      : m_loc(std::ref("<native code>"), 0), m_value(D_string(stdex.what()))
      {
      }
    ROCKET_COPYABLE_DESTRUCTOR(Traceable_Exception);

  public:
    const Source_Location & get_location() const noexcept
      {
        return this->m_loc;
      }
    const Value & get_value() const noexcept
      {
        return this->m_value;
      }

    const Cow_Vector<Source_Location> & get_backtrace() const noexcept
      {
        return this->m_backtrace;
      }
    void append_backtrace(const Source_Location &loc)
      {
        this->m_backtrace.emplace_back(loc);
      }

    const char * what() const noexcept override
      {
        return "Asteria::Traceable_Exception";
      }
  };

}

#endif