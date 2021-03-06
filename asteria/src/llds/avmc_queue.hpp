// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LLDS_AVMC_QUEUE_HPP_
#define ASTERIA_LLDS_AVMC_QUEUE_HPP_

#include "../fwd.hpp"
#include "../runtime/enums.hpp"
#include "../source_location.hpp"

namespace Asteria {

class AVMC_Queue
  {
  public:
    // This union can be used to encapsulate trivial information in solidified nodes.
    // At most 48 btis can be stored here. You may make appropriate use of them.
    // Fields of each struct here share a unique prefix. This helps you ensure that you don't
    // access fields of different structs at the same time.
    union ParamU
      {
        struct {
          uint16_t x_DO_NOT_USE_;
          uint16_t x16;
          uint32_t x32;
        };
        struct {
          uint16_t y_DO_NOT_USE_;
          uint8_t y8s[2];
          uint32_t y32;
        };
        struct {
          uint16_t u_DO_NOT_USE_;
          uint8_t u8s[6];
        };
        struct {
          uint16_t v_DO_NOT_USE_;
          uint16_t v16s[3];
        };
      };

    static_assert(sizeof(ParamU) == 8);

    // Symbols are optional. If no symbol is given, no backtrace frame is appended.
    // The source location is used to generate backtrace frames.
    struct Symbols
      {
        Source_Location sloc;
      };

    static_assert(::std::is_nothrow_copy_constructible<Symbols>::value);

    // These are prototypes for callbacks.
    using Constructor  = void (ParamU paramu, void* paramv, intptr_t source);
    using Destructor   = void (ParamU paramu, void* paramv);
    using Executor     = AIR_Status (Executive_Context& ctx, ParamU paramu, const void* paramv);
    using Enumerator   = Variable_Callback& (Variable_Callback& callback, ParamU paramu, const void* paramv);

  private:
    struct Vtable
      {
        Destructor* dtor;
        Executor* exec;
        Enumerator* vnum;
      };

    struct Header;

    Header* m_bptr = nullptr;  // beginning of raw storage
    uint32_t m_rsrv = 0;  // size of raw storage, in number of `Header`s [!]
    uint32_t m_used = 0;  // size of used storage, in number of `Header`s [!]

  public:
    constexpr
    AVMC_Queue()
    noexcept
      { }

    AVMC_Queue(AVMC_Queue&& other)
    noexcept
      { this->swap(other);  }

    AVMC_Queue&
    operator=(AVMC_Queue&& other)
    noexcept
      { return this->swap(other);  }

    ~AVMC_Queue()
      {
        if(this->m_bptr)
          this->do_deallocate_storage();
#ifdef ROCKET_DEBUG
        ::std::memset(static_cast<void*>(this), 0xCA, sizeof(*this));
#endif
      }

  private:
    void
    do_deallocate_storage();

    // Reserve storage for another node. `nbytes` is the size of `paramv` to reserve in bytes.
    // Note: All calls to this function must precede calls to `do_check_node_storage()`.
    void
    do_reserve_delta(size_t nbytes, const Symbols* syms_opt);

    // Allocate storage for all nodes that have been reserved so far, then checks whether there is enough room
    // for a new node with `paramv` whose size is `nbytes` in bytes. An exception is thrown in case of failure.
    inline
    Header*
    do_allocate_node(ParamU paramu, const Symbols* syms_opt, size_t nbytes);

    // Append a new node to the end. `nbytes` is the size of `paramv` to initialize in bytes.
    // Note: The storage must have been reserved using `do_reserve_delta()`.
    void
    do_append_trivial(Executor* exec, ParamU paramu, const Symbols* syms_opt,
                      const void* source_opt, size_t nbytes);

    void
    do_append_nontrivial(const Vtable* vtbl, ParamU paramu, const Symbols* syms_opt,
                         size_t nbytes, Constructor* ctor_opt, intptr_t source);

    template<Executor execT, nullptr_t, typename XNodeT>
    void
    do_dispatch_append(::std::true_type, ParamU paramu, const Symbols* syms_opt, XNodeT&& xnode)
      {
        // The parameter type is trivial and no vtable is required.
        // Append a node with a trivial parameter.
        this->do_append_trivial(execT, paramu, syms_opt, ::std::addressof(xnode), sizeof(xnode));
      }

    template<Executor execT, Enumerator* vnumT, typename XNodeT>
    void
    do_dispatch_append(::std::false_type, ParamU paramu, const Symbols* syms_opt, XNodeT&& xnode)
      {
        // The vtable must have static storage duration. As it is defined `constexpr` here, we need 'real'
        // function pointers. Those converted from non-capturing lambdas are not an option.
        struct Funcs
          {
            static
            void
            construct(ParamU /*paramu*/, void* paramv, intptr_t source)
              { ::rocket::construct_at((typename ::rocket::remove_cvref<XNodeT>::type*)paramv,
                             ::std::forward<XNodeT>(*(typename ::std::remove_reference<XNodeT>::type*)source));  }

            static
            void
            destroy(ParamU /*paramu*/, void* paramv)
              { ::rocket::destroy_at((typename ::rocket::remove_cvref<XNodeT>::type*)paramv);  }
          };

        // Define the virtual table.
        static constexpr Vtable s_vtbl = { Funcs::destroy, execT, vnumT };

        // Append a node with a non-trivial parameter.
        this->do_append_nontrivial(&s_vtbl, paramu, syms_opt,
                                   sizeof(xnode), Funcs::construct, (intptr_t)::std::addressof(xnode));
      }

  public:
    bool
    empty()
    const noexcept
      { return this->m_rsrv == 0;  }

    AVMC_Queue&
    clear()
    noexcept
      {
        if(this->m_bptr)
          this->do_deallocate_storage();

        // Clean invalid data up.
        this->m_bptr = nullptr;
        this->m_rsrv = 0;
        this->m_used = 0;
        return *this;
      }

    AVMC_Queue&
    swap(AVMC_Queue& other)
    noexcept
      {
        xswap(this->m_bptr, other.m_bptr);
        xswap(this->m_rsrv, other.m_rsrv);
        xswap(this->m_used, other.m_used);
        return *this;
      }

    AVMC_Queue&
    request(size_t nbytes, const Symbols* syms_opt)
      {
        // Reserve space for a node of size `nbytes`.
        this->do_reserve_delta(nbytes, syms_opt);
        return *this;
      }

    template<Executor execT>
    AVMC_Queue&
    append(ParamU paramu, const Symbols* syms_opt)
      {
        // Append a node with no parameter.
        this->do_append_trivial(execT, paramu, syms_opt, nullptr, 0);
        return *this;
      }

    template<Executor execT, typename XNodeT>
    AVMC_Queue&
    append(ParamU paramu, const Symbols* syms_opt, XNodeT&& xnode)
      {
        // Append a node with a parameter of type `remove_cvref_t<XNodeT>`.
        this->do_dispatch_append<execT, nullptr>(::std::is_trivial<typename ::std::remove_reference<XNodeT>::type>(),
                                                 paramu, syms_opt, ::std::forward<XNodeT>(xnode));
        return *this;
      }

    template<Executor execT, Enumerator vnumT, typename XNodeT>
    AVMC_Queue&
    append(ParamU paramu, const Symbols* syms_opt, XNodeT&& xnode)
      {
        // Append a node with a parameter of type `remove_cvref_t<XNodeT>`.
        this->do_dispatch_append<execT, vnumT>(::std::false_type(),  // cannot be trivial
                                               paramu, syms_opt, ::std::forward<XNodeT>(xnode));
        return *this;
      }

    AVMC_Queue&
    append_trivial(Executor* exec, ParamU paramu, const Symbols* syms_opt, const void* data, size_t size)
      {
        // Append an arbitrary function with a trivial argument.
        this->do_append_trivial(exec, paramu, syms_opt, data, size);
        return *this;
      }

    AVMC_Queue&
    reload(const cow_vector<AIR_Node>& code);

    AIR_Status
    execute(Executive_Context& ctx)
    const;

    Variable_Callback&
    enumerate_variables(Variable_Callback& callback)
    const;
  };

inline
void
swap(AVMC_Queue& lhs, AVMC_Queue& rhs)
noexcept
  { lhs.swap(rhs);  }

}  // namespace Asteria

#endif
