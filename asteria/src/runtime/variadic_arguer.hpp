// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_VARIADIC_ARGUER_HPP_
#define ASTERIA_RUNTIME_VARIADIC_ARGUER_HPP_

#include "../fwd.hpp"
#include "reference.hpp"
#include "../source_location.hpp"

namespace Asteria {

class Variadic_Arguer
final
  : public Abstract_Function
  {
  private:
    Source_Location m_sloc;
    cow_string m_func;
    cow_vector<Reference> m_vargs;

  public:
    template<typename... XVargsT>
    Variadic_Arguer(const Source_Location& xsloc, const cow_string& xfunc, XVargsT&&... xvargs)
      : m_sloc(xsloc), m_func(xfunc),
        m_vargs(::std::forward<XVargsT>(xvargs)...)
      { }

    template<typename... XVargsT>
    Variadic_Arguer(const Variadic_Arguer& other, XVargsT&&... xvargs)
      : m_sloc(other.m_sloc), m_func(other.m_func),
        m_vargs(::std::forward<XVargsT>(xvargs)...)
      { }

  public:
    const Source_Location&
    sloc()
    const noexcept
      { return this->m_sloc;  }

    const cow_string&
    file()
    const noexcept
      { return this->m_sloc.file();  }

    int
    line()
    const noexcept
      { return this->m_sloc.line();  }

    int
    offset()
    const noexcept
      { return this->m_sloc.offset();  }

    const cow_string&
    func()
    const noexcept
      { return this->m_func;  }

    bool
    empty()
    const noexcept
      { return this->m_vargs.empty();  }

    size_t
    size()
    const noexcept
      { return this->m_vargs.size();  }

    const Reference&
    arg(size_t index)
    const
      { return this->m_vargs.at(index);  }

    tinyfmt&
    describe(tinyfmt& fmt)
    const override;

    Variable_Callback&
    enumerate_variables(Variable_Callback& callback)
    const override;

    Reference&
    invoke_ptc_aware(Reference& self, Global_Context& global, cow_vector<Reference>&& args)
    const override;
  };

}  // namespace Asteria

#endif
