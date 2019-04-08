// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "_test_init.hpp"
#include "../asteria/src/compiler/simple_source_file.hpp"
#include "../asteria/src/runtime/global_context.hpp"
#include <sstream>

using namespace Asteria;

int main()
  {
    static constexpr char s_source[] =
      R"__(
        var b = false, i = 12, r = 8.5, s = "a";
        var a = [ 1, 2, 3 ];
        var o = { one: 1, two: 2, three: 3 };

        assert ++i == 13;
        assert i == 13;
        assert ++r == 9.5;
        assert r == 9.5;

        assert --i == 12;
        assert i == 12;
        assert --r == 8.5;
        assert r == 8.5;

        assert a[0] == 1;
        assert a[1] == 2;
        assert a[2] == 3;
        assert a[3] == null;
        assert a[-1] == 3;
        assert a[-2] == 2;
        assert a[-3] == 1;
        assert a[-4] == null;
        assert o["one"] == 1;
        assert o["two"] == 2;
        assert o["three"] == 3;
        assert o["nonexistent"] == null;

        assert o.one == 1;
        assert o.two == 2;
        assert o.three == 3;
        assert o.nonexistent == null;

        assert +b == false;
        assert +i == 12;
        assert +r == 8.5;
        assert +s == 'a';

        assert -i == -12;
        assert -r == -8.5;

        assert ~b == true;
        assert ~i == -13;

        assert !b == true;
        assert !i == false;
        assert !r == false;
        assert !s == false;
        assert !a == false;
        assert !o == false;
        assert !0 == true;
        assert !0.0 == true;
        assert !"" == true;
        assert ![] == true;
        assert !{} == false;

        assert i++ == 12;
        assert i == 13;
        assert r++ == 8.5;
        assert r == 9.5;

        assert i-- == 13;
        assert i == 12;
        assert r-- == 9.5;
        assert r == 8.5;

        assert unset a[1] == 2;
        assert a == [1, 3];
        assert unset a[10000] == null;
        assert unset o["one"] == 1;
        assert o.one == null;
        assert unset o.nonexistent == null;

        assert i * 3 == 36;
        assert 3 * i == 36;
        assert r * 3.0 == 25.5;
        assert 3.0 * r == 25.5;
        assert r * 5 == 42.5;
        assert 5 * r == 42.5;
        assert s * 3 == 'aaa';
        assert 3 * s == 'aaa';

        assert i / 5 == 2;
        assert 23 / i == 1;
        assert r / 2.0 == 4.25;
        assert 17.0 / r == 2.0;
        assert r / 5 == 1.7;
        assert 17 / 8.5 == 2.0;

        assert i % 7 == 5;
        assert 23 % i == 11;
        assert r % 1.125 == 0.625;
        assert 19.5 % r == 2.5;
        assert r % 2 == 0.5;
        assert 10 % r == 1.5;

        assert b + false == false;
        assert false + b == false;
        assert b + true == true;
        assert true + b == true;
        assert i + 2 == 14;
        assert 2 + i == 14;
        assert r + 2.0 == 10.5;
        assert 2.0 + r == 10.5;
        assert r + 3 == 11.5;
        assert 3 + r == 11.5;
        assert s + 'bc' == 'abc';
        assert 'bc' + s == 'bca';

        assert i - 3 == 9;
        assert 3 - i == -9;
        assert r - 3.25 == 5.25;
        assert 3.25 - r == -5.25;
        assert r - 3 == 5.5;
        assert 3 - r == -5.5;

        assert i <<< 3 == 96;
        assert -10 <<< 1 == -20;
        assert 'abc' <<< 1 == 'bc ';

        assert i >>> 3 == 1;
        assert -10 >>> 1 == 9223372036854775803;
        assert 'abc' >>> 1 == ' ab';

        assert i << 3 == 96;
        assert -10 << 1 == -20;
        assert 'abc' << 1 == 'abc ';

        assert i >> 3 == 1;
        assert -10 >> 1 == -5;
        assert 'abc' >> 1 == 'ab';

        assert false < true;
        assert 1 < 2;
        assert 1.0 < 2.0;
        assert 1.0p30 < infinity;
        assert "aa" < "b";

        assert true > false;
        assert 2 > 1;
        assert 2.0 > 1.0;
        assert -1.0p30 > -infinity;
        assert "aa" > "a";

        assert true >= true;
        assert false >= false;
        assert -1 >= -1;
        assert -1 >= -2;
        assert 10.0 >= 9.9;
        assert "bb" >= "bb";

        assert true <= true;
        assert false <= false;
        assert -1 <= -1;
        assert -1 <= 0;
        assert 10.0 <= 10.1;
        assert "bb" <= "bb";

        assert true == true;
        assert false == false;
        assert -2 == -2;
        assert "cd" == "cd";

        assert false != true;
        assert 1 != 0;
        assert nan != nan;
        assert "abc" != "def";
        assert false != null;
        assert "" != null;
        assert [] != 0;
        assert {} != [];

        assert (1 <=> 2) == -1;
        assert ("b" <=> "a") == 1;
        assert (true <=> true) == 0;
        assert ("false" <=> false) == "<unordered>";

        assert (true & true) == true;
        assert (false & true) == false;
        assert (true & false) == false;
        assert (false & false) == false;
        assert (5 & 4) == 4;
        assert (-1 & -2) == -2;

        assert (true ^ true) == false;
        assert (false ^ true) == true;
        assert (true ^ false) == true;
        assert (false ^ false) == false;
        assert (5 ^ 4) == 1;
        assert (-1 ^ -2) == 1;

        assert (true | true) == true;
        assert (false | true) == true;
        assert (true | false) == true;
        assert (false | false) == false;
        assert (5 | 4) == 5;
        assert (-1 | -2) == -1;

        assert (1 && ++i) == 13;
        assert i == 13;
        assert (0 && ++i) == 0;
        assert i == 13;

        assert (1 || --i) == 1;
        assert i == 13;
        assert (0 || --i) == 12;
        assert i == 12;

        assert i ?? "abc" == 12;
        assert null ?? "abc" == "abc";
        assert null ?? null ?? 1 ?? null ?? 2 == 1;
      )__";

    std::istringstream iss(s_source);
    Simple_Source_File code(iss, rocket::sref("my_file"));
    Global_Context global;
    code.execute(global, { });
  }
