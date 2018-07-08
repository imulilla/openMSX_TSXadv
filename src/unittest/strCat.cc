#include "catch.hpp"
#include "strCat.hh"

static std::string rValue()
{
	return "bar";
}

struct MyType
{
	char c;
};

static std::ostream& operator<<(std::ostream& os, const MyType& m)
{
	os << "--|" << m.c << "|--";
	return os;
}

TEST_CASE("strCat")
{
	std::string str = "abc";
	string_ref sr = "xyz";
	const char* literal = "foo";
	char buf[100]; buf[0] = 'q'; buf[1] = 'u'; buf[2] = 'x'; buf[3] = '\0';
	char c = '-';
	unsigned char uc = 222;
	int m = -31;
	int i = 123456;
	float f = 6.28f;
	double d = -1.234;
	MyType t = {'a'};

	SECTION("zero") {
		CHECK(strCat() == "");
	}
	SECTION("one") {
		CHECK(strCat(rValue()) == "bar");
		CHECK(strCat(str) == "abc");
		CHECK(strCat(sr) == "xyz");
		CHECK(strCat(literal) == "foo");
		CHECK(strCat(buf) == "qux");
		CHECK(strCat(c) == "-");
		CHECK(strCat(uc) == "222");
		CHECK(strCat(m) == "-31");
		CHECK(strCat(i) == "123456");
		CHECK(strCat(f) == "6.280000");
		CHECK(strCat(d) == "-1.234000");
		CHECK(strCat(t) == "--|a|--");
		CHECK(strCat(hex_string<8>(i)) == "0001e240");
		CHECK(strCat(hex_string<4>(i)) == "e240");
		CHECK(strCat(spaces(5)) == "     ");
	}
	SECTION("two") {
		CHECK(strCat(str, i) == "abc123456");
		CHECK(strCat(str, m) == "abc-31");
		CHECK(strCat(str, uc) == "abc222");
		CHECK(strCat(i, str) == "123456abc");
		CHECK(strCat(m, str) == "-31abc");
		CHECK(strCat(uc, str) == "222abc");
		CHECK(strCat(buf, i) == "qux123456");
		CHECK(strCat(literal, m) == "foo-31");
		CHECK(strCat(i, m) == "123456-31");
		CHECK(strCat(c, m) == "--31");
		CHECK(strCat(i, rValue()) == "123456bar");
		CHECK(strCat(spaces(4), i) == "    123456");
		CHECK(strCat(hex_string<3>(i), i) == "240123456");
		CHECK(strCat(c, hex_string<1>(i)) == "-0");
		CHECK(strCat(rValue(), uc) == "bar222");

		// these have special overloads
		CHECK(strCat(str, str) == "abcabc");
		CHECK(strCat(literal, str) == "fooabc");
		CHECK(strCat(c, str) == "-abc");
		CHECK(strCat(str, literal) == "abcfoo");
		CHECK(strCat(str, c) == "abc-");
		CHECK(strCat(rValue(), str) == "barabc");
		CHECK(strCat(str, rValue()) == "abcbar");
		CHECK(strCat(rValue(), rValue()) == "barbar");
		CHECK(strCat(literal, rValue()) == "foobar");
		CHECK(strCat(c, rValue()) == "-bar");
		CHECK(strCat(rValue(), literal) == "barfoo");
		CHECK(strCat(rValue(), c) == "bar-");
		CHECK(strCat(rValue(), sr) == "barxyz");
	}
	SECTION("three") {
		CHECK(strCat(str, str, str) == "abcabcabc");
		CHECK(strCat(i, m, c) == "123456-31-");
		CHECK(strCat(literal, buf, rValue()) == "fooquxbar");

		// 1st an r-value is a special case
		CHECK(strCat(rValue(), i, literal) == "bar123456foo");
		CHECK(strCat(rValue(), uc, buf) == "bar222qux");
	}
	SECTION("many") {
		CHECK(strCat(str, sr, literal, buf, c, uc, m, i, rValue(), spaces(2), hex_string<2>(255)) ==
		      "abcxyzfooqux-222-31123456bar  ff");
		CHECK(strCat(rValue(), uc, buf, c, spaces(2), str, i, hex_string<3>(9999), sr, literal, m) ==
		      "bar222qux-  abc12345670fxyzfoo-31");
	}
}

template<typename... Args>
static void testAppend(const std::string& expected, Args&& ...args)
{
	std::string s1;
	strAppend(s1, std::forward<Args>(args)...);
	CHECK(s1 == expected);

	std::string s2 = "abcdefghijklmnopqrstuvwxyz";
	strAppend(s2, std::forward<Args>(args)...);
	CHECK(s2 == ("abcdefghijklmnopqrstuvwxyz" + expected));
}

TEST_CASE("strAppend")
{
	std::string str = "mno";
	string_ref sr = "rst";
	const char* literal = "ijklmn";
	char buf[100]; buf[0] = 'd'; buf[1] = 'e'; buf[2] = '\0'; buf[3] = 'f';
	char c = '+';
	unsigned u = 4294967295u;
	long long ll = -876;

	SECTION("zero") {
		testAppend("");
	}
	SECTION("one") {
		testAppend("bar", rValue());
		testAppend("mno", str);
		testAppend("rst", sr);
		testAppend("ijklmn", literal);
		testAppend("de", buf);
		testAppend("+", c);
		testAppend("4294967295", u);
		testAppend("-876", ll);
		testAppend("          ", spaces(10));
		testAppend("fffffffc94", hex_string<10>(ll));
	}
	SECTION("many") {
		std::string s = "bla";
		strAppend(s, str, sr, literal, spaces(3));
		strAppend(s, buf, c, u, ll, rValue());
		CHECK(s == "blamnorstijklmn   de+4294967295-876bar");
	}
}


#if 0

// Not part of the actual unittest. Can be used to (manually) inspect the
// quality of the generated code.

auto test1(int i) { return strCat(i); }
auto test1b(int i) { return std::to_string(i); }
auto test2(const char* s) { return strCat(s); }
auto test3(string_ref s) { return strCat(s); }
auto test4(const std::string& s) { return strCat(s); }
auto test5() { return strCat("bla"); }
auto test6() { return strCat('a'); }
auto test7(char i) { return strCat('a', i, "bla"); }
auto test8(int i, unsigned u) { return strCat(i, u); }

auto testA1(const std::string& s1, const std::string& s2) { return strCat(s1, s2); }
auto testA2(const std::string& s1, const std::string& s2) { return s1 + s2; }

#endif