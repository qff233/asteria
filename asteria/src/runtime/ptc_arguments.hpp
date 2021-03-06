// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_PTC_ARGUMENTS_HPP_
#define ASTERIA_RUNTIME_PTC_ARGUMENTS_HPP_

#include "../fwd.hpp"
#include "variadic_arguer.hpp"
#include "../source_location.hpp"

namespace Asteria {

class PTC_Arguments
final
  : public Rcfwd<PTC_Arguments>
  {
  private:
    // These describe characteristics of the function call.
    Source_Location m_sloc;
    rcptr<Variadic_Arguer> m_zvarg;
    PTC_Aware m_ptc;

    // These are deferred expressions.
    cow_bivector<Source_Location, AVMC_Queue> m_defer;

    // This is the target function.
    cow_function m_target;
    // The last reference is `self`.
    // We can't store a `Reference` directly as the class `Reference` is incomplete here.
    cow_vector<Reference> m_args_self;

  public:
    PTC_Arguments(const Source_Location& sloc, const rcptr<Variadic_Arguer>& zvarg, PTC_Aware ptc,
                  const cow_function& target, cow_vector<Reference>&& args_self)
      : m_sloc(sloc), m_zvarg(zvarg), m_ptc(ptc),
        m_target(target), m_args_self(::std::move(args_self))
      { }

    ~PTC_Arguments()
    override;

    PTC_Arguments(const PTC_Arguments&)
      = delete;

    PTC_Arguments&
    operator=(const PTC_Arguments&)
      = delete;

  public:
    const Source_Location&
    sloc()
    const noexcept
      { return this->m_sloc;  }

    const rcptr<Variadic_Arguer>&
    zvarg()
    const noexcept
      { return this->m_zvarg;  }

    PTC_Aware
    ptc_aware()
    const noexcept
      { return this->m_ptc;  }

    const cow_bivector<Source_Location, AVMC_Queue>&
    get_defer_stack()
    const noexcept
      { return this->m_defer;  }

    cow_bivector<Source_Location, AVMC_Queue>&
    open_defer_stack()
    noexcept
      { return this->m_defer;  }

    const cow_function&
    get_target()
    const noexcept
      { return this->m_target;  }

    const cow_vector<Reference>&
    get_arguments_and_self()
    const noexcept
      { return this->m_args_self;  }

    cow_vector<Reference>&
    open_arguments_and_self()
    noexcept
      { return this->m_args_self;  }

    Variable_Callback&
    enumerate_variables(Variable_Callback& callback)
    const;
  };

}  // namespace Asteria

#endif
