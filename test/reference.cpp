// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "_test_init.hpp"
#include "../src/reference.hpp"

using namespace Asteria;

int main()
  {
    auto ref = Reference(Reference_root::S_constant { D_string(String::shallow("meow")) });
    auto val = ref.read();
    ASTERIA_TEST_CHECK(val.type() == Value::type_string);
    ASTERIA_TEST_CHECK(val.check<D_string>() == "meow");
    ASTERIA_TEST_CHECK_CATCH(ref.write(D_boolean(true)));
    auto ref2 = ref;
    val = ref2.read();
    ASTERIA_TEST_CHECK(val.type() == Value::type_string);
    ASTERIA_TEST_CHECK(val.check<D_string>() == "meow");
    ASTERIA_TEST_CHECK_CATCH(ref.write(D_boolean(true)));

    ref = Reference_root::S_temporary { D_integer(42) };
    val = ref.read();
    ASTERIA_TEST_CHECK(val.type() == Value::type_integer);
    ASTERIA_TEST_CHECK(val.check<D_integer>() == 42);
    ASTERIA_TEST_CHECK_CATCH(ref.write(D_boolean(true)));
    val = ref2.read();
    ASTERIA_TEST_CHECK(val.type() == Value::type_string);
    ASTERIA_TEST_CHECK(val.check<D_string>() == "meow");
    ASTERIA_TEST_CHECK_CATCH(ref.write(D_boolean(true)));

    ref.materialize();
    val = ref.read();
    ASTERIA_TEST_CHECK(val.type() == Value::type_integer);
    ASTERIA_TEST_CHECK(val.check<D_integer>() == 42);
    ref.write(D_boolean(true));
    val = ref.read();
    ASTERIA_TEST_CHECK(val.type() == Value::type_boolean);
    ASTERIA_TEST_CHECK(val.check<D_boolean>() == true);

    ref = Reference_root::S_temporary { D_null() };
    ref.materialize();
    ref.zoom_in(Reference_modifier::S_array_index { -3 });
    val = ref.read();
    ASTERIA_TEST_CHECK(val.type() == Value::type_null);
    ref.write(D_integer(36));
    ref.zoom_out();
    ref.zoom_in(Reference_modifier::S_array_index { 0 });
    val = ref.read();
    ASTERIA_TEST_CHECK(val.type() == Value::type_integer);
    ASTERIA_TEST_CHECK(val.check<D_integer>() == 36);
    ref.zoom_out();

    ref.zoom_in(Reference_modifier::S_array_index { 2 });
    ref.zoom_in(Reference_modifier::S_object_key { String::shallow("my_key") });
    val = ref.read();
    ASTERIA_TEST_CHECK(val.type() == Value::type_null);
    ref.write(D_double(10.5));
    val = ref.read();
    ASTERIA_TEST_CHECK(val.type() == Value::type_double);
    ASTERIA_TEST_CHECK(val.check<D_double>() == 10.5);
    ref.zoom_out();
    ref.zoom_out();
    ref.zoom_in(Reference_modifier::S_array_index { -1 });
    ref.zoom_in(Reference_modifier::S_object_key { String::shallow("my_key") });
    val = ref.read();
    ASTERIA_TEST_CHECK(val.type() == Value::type_double);
    ASTERIA_TEST_CHECK(val.check<D_double>() == 10.5);
    ref.zoom_in(Reference_modifier::S_object_key { String::shallow("invalid_access") });
    ASTERIA_TEST_CHECK_CATCH(val = ref.read());
    ref.zoom_out();

    val = ref.unset();
    ASTERIA_TEST_CHECK(val.type() == Value::type_double);
    ASTERIA_TEST_CHECK(val.check<D_double>() == 10.5);
    val = ref.read();
    ASTERIA_TEST_CHECK(val.type() == Value::type_null);
    val = ref.unset();
    ASTERIA_TEST_CHECK(val.type() == Value::type_null);
  }
