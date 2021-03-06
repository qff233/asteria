// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_GENIUS_COLLECTOR_HPP_
#define ASTERIA_RUNTIME_GENIUS_COLLECTOR_HPP_

#include "../fwd.hpp"
#include "collector.hpp"
#include "../llds/variable_hashset.hpp"

namespace Asteria {

class Genius_Collector
final
  : public Rcfwd<Genius_Collector>
  {
  private:
    // Mind the order of construction and destruction.
    Variable_HashSet m_pool;
    Collector m_oldest;
    Collector m_middle;
    Collector m_newest;

  public:
    Genius_Collector()
    noexcept
      : m_oldest(&(this->m_pool),           nullptr,  10),
        m_middle(&(this->m_pool), &(this->m_oldest),  60),
        m_newest(&(this->m_pool), &(this->m_middle), 800)
      { }

    ~Genius_Collector()
    override;

    Genius_Collector(const Genius_Collector&)
      = delete;

    Genius_Collector&
    operator=(const Genius_Collector&)
      = delete;

  private:
    Collector Genius_Collector::*
    do_locate(GC_Generation gc_gen)
    const;

  public:
    size_t
    get_pool_size()
    const noexcept
      { return this->m_pool.size();  }

    Genius_Collector&
    clear_pool()
    noexcept
      { return this->m_pool.clear(), *this;  }

    const Collector&
    get_collector(GC_Generation gc_gen)
    const
      { return this->*(this->do_locate(gc_gen));  }

    Collector&
    open_collector(GC_Generation gc_gen)
      { return this->*(this->do_locate(gc_gen));  }

    rcptr<Variable>
    create_variable(GC_Generation gc_hint = gc_generation_newest);

    size_t
    collect_variables(GC_Generation gc_limit = gc_generation_oldest);

    Genius_Collector&
    wipe_out_variables()
    noexcept;
  };

}  // namespace Asteria

#endif
