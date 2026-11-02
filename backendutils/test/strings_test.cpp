#include "test_support.h"
using namespace simpleutils;
TEST(Strings, BinaryRoundtrip) {
  std::string s("a\0b\xff", 4);
  EXPECT_EQ(toString(toBytes(s)), s);
  EXPECT_EQ(toString({}), "");
}
TEST(Strings, TrimAscii) {
  EXPECT_EQ(trim(" \t\r\nabc\v\f "), "abc");
  EXPECT_EQ(trim(" \t"), "");
  EXPECT_EQ(trim(""), "");
  EXPECT_EQ(trim("a b"), "a b");
}
TEST(Strings, CaseAscii) {
  EXPECT_EQ(toLower("AbC09_"), "abc09_");
  EXPECT_EQ(toUpper("AbC09_"), "ABC09_");
  std::string s("\xff", 1);
  EXPECT_EQ(toLower(s), s);
  EXPECT_EQ(toUpper(s), s);
}
TEST(Strings, PrefixSuffix) {
  EXPECT_TRUE(startsWith("abc", "ab"));
  EXPECT_TRUE(endsWith("abc", "bc"));
  EXPECT_TRUE(startsWith("", ""));
  EXPECT_TRUE(endsWith("abc", ""));
  EXPECT_FALSE(startsWith("a", "ab"));
  EXPECT_FALSE(endsWith("a", "ba"));
}
TEST(Strings, Replace) {
  EXPECT_EQ(replaceAll("aaaa", "aa", "b"), "bb");
  EXPECT_EQ(replaceAll("a", "a", "aa"), "aa");
  EXPECT_EQ(replaceAll("abc", "z", "q"), "abc");
  EXPECT_EQ(replaceAll("abc", "b", ""), "ac");
  EXPECT_THROW(replaceAll("abc", "", "z"), std::invalid_argument);
}
TEST(Strings, SplitJoin) {
  EXPECT_EQ(split(",a,,", ","), (std::vector<std::string>{"", "a", "", ""}));
  EXPECT_EQ(split(",a,,", ",", false), (std::vector<std::string>{"a"}));
  EXPECT_EQ(split("a<>b<>", "<>"), (std::vector<std::string>{"a", "b", ""}));
  EXPECT_EQ(split("", ","), (std::vector<std::string>{""}));
  EXPECT_TRUE(split("", ",", false).empty());
  EXPECT_EQ(join({"a", "", "c"}, "|"), "a||c");
  EXPECT_EQ(join({}, ","), "");
  EXPECT_THROW(split("a", ""), std::invalid_argument);
}
TEST(Strings, FindSemantics) {
  EXPECT_EQ(find("abcabc", "bc"), 1u);
  EXPECT_EQ(find("abc", "abc"), 0u);
  EXPECT_EQ(find("abc", ""), 0u);
  EXPECT_EQ(find("", "a"), std::string::npos);
  EXPECT_EQ(find("ab", "abc"), std::string::npos);
  EXPECT_THROW(find(nullptr, "a"), std::invalid_argument);
  EXPECT_THROW(find("a", nullptr), std::invalid_argument);
}
TEST(Strings, Paths) {
  EXPECT_EQ(ExtractPathOnly("a/b.txt"), "a");
  EXPECT_EQ(ExtractPathOnly("/file"), "/");
  EXPECT_EQ(ExtractPathOnly("x"), "");
  EXPECT_EQ(ExtractFileOnly("C:\\dir\\a.txt"), "a.txt");
  EXPECT_EQ(ExtractPathOnly("C:\\dir\\a.txt"), "C:\\dir");
  EXPECT_EQ(ExtractFileOnly("a/"), "");
  EXPECT_EQ(ExtractFileOnly("x"), "x");
}
TEST(Strings, HexAllBytes) {
  std::vector<uint8_t> b;
  for (int i = 0; i < 256; ++i) b.push_back(i);
  EXPECT_EQ(fromHex(toHex(b)), b);
  EXPECT_EQ(fromHex(toHex(b, true)), b);
  EXPECT_EQ(toHex({0, 255}), "00ff");
  EXPECT_EQ(toHex({0, 255}, true), "00FF");
  EXPECT_TRUE(fromHex("").empty());
}
class BadHex : public testing::TestWithParam<const char*> {};
TEST_P(BadHex, Reject) { EXPECT_THROW(fromHex(GetParam()), std::invalid_argument); }
INSTANTIATE_TEST_SUITE_P(Malformed, BadHex,
                         testing::Values("0", "fff", "0g", "gg", " 0", "0xFF", "-1"));
TEST(Files, TextAndBinary) {
  test_support::TempDir d;
  auto p = d.file("data");
  std::string s("a\0b", 3);
  WriteFile(p, s);
  EXPECT_EQ(ReadFile(p), s);
  EXPECT_EQ(ReadBinaryFile(p), toBytes(s));
  WriteFile(p, "");
  EXPECT_EQ(ReadFile(p), "");
  EXPECT_THROW(ReadFile(d.file("missing")), std::runtime_error);
  EXPECT_THROW(WriteFile(d.file("missing/file"), "x"), std::runtime_error);
}
TEST(CSV, EmptyAndTrailing) {
  EXPECT_TRUE(parseCSVText("").empty());
  EXPECT_EQ(splitCSV(""), (std::vector<std::string>{""}));
  EXPECT_EQ(splitCSV("a,,"), (std::vector<std::string>{"a", "", ""}));
  EXPECT_EQ(parseCSVText("a,b\r\nc,d\r\n"),
            (std::vector<std::vector<std::string>>{{"a", "b"}, {"c", "d"}}));
  EXPECT_EQ(parseCSVText("\n"), (std::vector<std::vector<std::string>>{{""}}));
}
TEST(CSV, QuotesMultiline) {
  EXPECT_EQ(splitCSV("\"a,b\",\"say \"\"hi\"\"\",x"),
            (std::vector<std::string>{"a,b", "say \"hi\"", "x"}));
  EXPECT_EQ(parseCSVText("\"a\nb\",c\nd,e"),
            (std::vector<std::vector<std::string>>{{"a\nb", "c"}, {"d", "e"}}));
  EXPECT_EQ(splitCSV("a;b;", ';'), (std::vector<std::string>{"a", "b", ""}));
  EXPECT_EQ(parseCSVText("\"\""), (std::vector<std::vector<std::string>>{{""}}));
}
class BadCsv : public testing::TestWithParam<const char*> {};
TEST_P(BadCsv, Reject) { EXPECT_THROW(parseCSVText(GetParam()), std::invalid_argument); }
INSTANTIATE_TEST_SUITE_P(Malformed, BadCsv,
                         testing::Values("\"abc", "ab\"c", "\"a\"x", "\"a\" \n"));
TEST(CSV, InvalidDelimiterAndRecords) {
  EXPECT_THROW(parseCSVText("x", '"'), std::invalid_argument);
  EXPECT_THROW(parseCSVText("x", '\n'), std::invalid_argument);
  EXPECT_THROW(splitCSV("a\nb"), std::invalid_argument);
}
TEST(CSV, File) {
  test_support::TempDir d;
  WriteFile(d.file("csv"), "a,b\n1,2");
  EXPECT_EQ(parseCSV(d.file("csv")).size(), 2u);
}
