// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "_test_init.hpp"
#include "../asteria/src/compiler/simple_source_file.hpp"
#include "../asteria/src/runtime/global_context.hpp"
#include <sstream>

using namespace Asteria;

std::atomic<long> bcnt;

void * operator new(std::size_t cb)
  {
    const auto ptr = std::malloc(cb);
    if(!ptr) {
      throw std::bad_alloc();
    }
    bcnt.fetch_add(1, std::memory_order_relaxed);
    return ptr;
  }

void operator delete(void *ptr) noexcept
  {
    if(!ptr) {
      return;
    }
    bcnt.fetch_sub(1, std::memory_order_relaxed);
    std::free(ptr);
  }

void operator delete(void *ptr, std::size_t) noexcept
  {
    operator delete(ptr);
  }

int main()
  {
    ASTERIA_TEST_CHECK(bcnt.load(std::memory_order_relaxed) == 0);
    {
      std::istringstream iss(R"__(
        var g;
        func leak() {
          var f = 1;
          g = func() { return f; };
          return g();
        }
        for(var i = 0; i < 10000; ++i) {
          leak();
        }
      )__");
      Simple_source_file code(iss, std::ref("my_file"));
      Global_context global;
      code.execute(global, { });
    }
    ASTERIA_TEST_CHECK(bcnt.load(std::memory_order_relaxed) == 0);
  }
