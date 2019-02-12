// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "_test_init.hpp"
#include "../asteria/src/compiler/simple_source_file.hpp"
#include "../asteria/src/runtime/global_context.hpp"
#include "../asteria/src/rocket/insertable_istream.hpp"

using namespace Asteria;

int main()
  {
    rocket::insertable_istream iss(
      rocket::sref(
        R"__(
          return std.meow;
        )__")
      );
    Simple_Source_File code(iss, rocket::sref("my_file"));
    Global_Context global;
    auto retv = code.execute(global, { }).read();
    ASTERIA_TEST_CHECK(retv.type() == type_null);

    global.open_std_member(rocket::sref("meow")) = D_integer(42);
    retv = code.execute(global, { }).read();
    ASTERIA_TEST_CHECK(retv.check<D_integer>() == 42);

    global.remove_std_member(rocket::sref("meow"));
    retv = code.execute(global, { }).read();
    ASTERIA_TEST_CHECK(retv.type() == type_null);
  }