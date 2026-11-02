#include "test_support.h"
using namespace simpleutils;
TEST(Buffer, SetAndIndex) {
  Buffer b;
  b.setData(std::vector<uint8_t>{1, 2});
  EXPECT_EQ(b.size(), 2u);
  b[1] = 3;
  const auto& c = b;
  EXPECT_EQ(c[1], 3);
  EXPECT_THROW(b[2], std::out_of_range);
  b.setData(b.data.data(), 2);
  EXPECT_EQ(b.data, (std::vector<uint8_t>{1, 3}));
  b.setData(static_cast<const uint8_t*>(nullptr), 0);
  EXPECT_EQ(b.size(), 0u);
  EXPECT_THROW(b.setData(static_cast<const uint8_t*>(nullptr), 1), std::invalid_argument);
  b.setData("ab", 2);
  EXPECT_EQ(b.data, toBytes("ab"));
}
TEST(Buffer, Mid) {
  Buffer b{{1, 2, 3}};
  std::vector<uint8_t> r;
  b.Mid(r, 0, 3);
  EXPECT_EQ(r, b.data);
  std::string s = "x";
  b.Mid(s, 1, 3);
  EXPECT_EQ(s, std::string("x\2\3", 3));
  b.Mid(r, 3, 3);
  EXPECT_EQ(r.size(), 3u);
  EXPECT_THROW(b.Mid(r, -1, 2), std::out_of_range);
  EXPECT_THROW(b.Mid(s, 2, 1), std::out_of_range);
  EXPECT_THROW(b.Mid(r, 0, 4), std::out_of_range);
  b.Mid(b.data, 0, 1);
  EXPECT_EQ(b.size(), 4u);
}
TEST(Buffer, InsertAllOverloads) {
  Buffer b;
  b.insertInto(std::vector<uint8_t>{2}, 0);
  b.insertInto(Buffer{{1}}, 0);
  b.insertInto(std::string("ab"), 2);
  b.insertInto("c", 4);
  EXPECT_EQ(b.data, (std::vector<uint8_t>{1, 2, 'a', 'b', 'c'}));
  EXPECT_THROW(b.insertInto("x", -1), std::out_of_range);
  EXPECT_THROW(b.insertInto("x", 6), std::out_of_range);
  EXPECT_THROW(b.insertInto(static_cast<const char*>(nullptr), 0), std::invalid_argument);
}
TEST(Buffer, RemoveAndPad) {
  Buffer b{{1, 2, 3}};
  b.removeFrom(1, 100);
  EXPECT_EQ(b.data, (std::vector<uint8_t>{1}));
  b.removeFrom(1, 0);
  EXPECT_THROW(b.removeFrom(0, -1), std::invalid_argument);
  EXPECT_THROW(b.removeFrom(2, 1), std::out_of_range);
  EXPECT_EQ(b.pad(3, 0, true), (std::vector<uint8_t>{0, 0, 1}));
  EXPECT_EQ(b.pad(4, 9), (std::vector<uint8_t>{0, 0, 1, 9}));
  EXPECT_EQ(b.pad(2, 4), b.data);
  b.append(8);
  EXPECT_EQ(b[4], 8);
}
static void visit(Buffer* b, unsigned char c, int pos) { b->data.at(pos) = c + 1; }
TEST(Buffer, Iterate) {
  Buffer b{{1, 2}};
  b.iterateOver(visit);
  EXPECT_EQ(b.data, (std::vector<uint8_t>{2, 3}));
  EXPECT_THROW(b.iterateOver(nullptr), std::invalid_argument);
}
TEST(Bytes, ReverseExhaustive) {
  for (int i = 0; i < 256; ++i) {
    auto reversed = ByteUtils::reverseByte(i);
    EXPECT_EQ(ByteUtils::reverseByte(reversed), i);
    int expected = 0;
    for (int bit = 0; bit < 8; ++bit)
      if (i & (1 << bit)) expected |= 1 << (7 - bit);
    EXPECT_EQ(reversed, expected);
  }
  EXPECT_EQ(ByteUtils::toHexString(ByteUtils::toBuffer(0xab01)), "ab01");
}
TEST(Containers, ContainsMapFilterFind) {
  std::vector<int> v{1, 2, 3};
  EXPECT_TRUE(contains(v, 2));
  EXPECT_FALSE(contains(v, 9));
  EXPECT_EQ(mapToVector(v, [](int x) { return std::to_string(x); }),
            (std::vector<std::string>{"1", "2", "3"}));
  EXPECT_EQ(filterToVector(v, [](int x) { return x % 2; }), (std::vector<int>{1, 3}));
  EXPECT_EQ(findOptional(v, [](int x) { return x > 1; }), 2);
  EXPECT_FALSE(findOptional(v, [](int x) { return x > 5; }));
  EXPECT_TRUE(mapToVector(std::vector<int>{}, [](int x) { return x; }).empty());
}
TEST(Containers, UniqueGroup) {
  EXPECT_EQ(uniqueStable(std::vector<int>{3, 1, 3, 2, 1}), (std::vector<int>{3, 1, 2}));
  auto r = groupBy(std::vector<int>{1, 2, 3, 4}, [](int x) { return x % 2; });
  EXPECT_EQ(r[1], (std::vector<int>{1, 3}));
  EXPECT_EQ(r[0], (std::vector<int>{2, 4}));
  EXPECT_TRUE(uniqueStable(std::vector<int>{}).empty());
}
TEST(Containers, ChunkZip) {
  EXPECT_EQ(chunk(std::vector<int>{1, 2, 3}, 2), (std::vector<std::vector<int>>{{1, 2}, {3}}));
  EXPECT_TRUE(chunk(std::vector<int>{}, 2).empty());
  EXPECT_EQ(chunk(std::vector<int>{1, 2}, 100).size(), 1u);
  EXPECT_THROW(chunk(std::vector<int>{}, 0), std::invalid_argument);
  auto r = zip(std::vector<int>{1, 2}, std::vector<std::string>{"a"});
  ASSERT_EQ(r.size(), 1u);
  EXPECT_EQ(r[0].first, 1);
  EXPECT_EQ(r[0].second, "a");
}
TEST(Containers, Dictionary) {
  std::map<std::string, int> m{{"a", 1}, {"b", 2}};
  EXPECT_EQ(keys(m), (std::vector<std::string>{"a", "b"}));
  EXPECT_EQ(values(m), (std::vector<int>{1, 2}));
  EXPECT_EQ(getOr(m, "a", 5), 1);
  EXPECT_EQ(getOr(m, "c", 5), 5);
  EXPECT_EQ(m.size(), 2u);
}
TEST(Functions, ScopeExitReleaseMoveUnwind) {
  int n = 0;
  {
    auto g = scopeExit([&] { ++n; });
  }
  EXPECT_EQ(n, 1);
  {
    auto g = scopeExit([&] { ++n; });
    g.release();
  }
  EXPECT_EQ(n, 1);
  {
    auto g = scopeExit([&] { ++n; });
    auto h = std::move(g);
  }
  EXPECT_EQ(n, 2);
  try {
    auto g = scopeExit([&] { ++n; });
    throw 1;
  } catch (int) {
  }
  EXPECT_EQ(n, 3);
}
TEST(Functions, ComposePipe) {
  auto plus = [](int x) { return x + 1; };
  auto twice = [](int x) { return x * 2; };
  EXPECT_EQ(compose(plus, twice)(3), 7);
  EXPECT_EQ(pipe(plus, twice)(3), 8);
  auto sum = [](int a, int b) { return a + b; };
  EXPECT_EQ(compose(twice, sum)(2, 3), 10);
}
TEST(Regex, ValidInvalid) {
  EXPECT_TRUE(validateExpression("123", "[0-9]+"));
  EXPECT_FALSE(validateExpression("123x", "[0-9]+"));
  EXPECT_FALSE(validateExpression("x", "["));
  EXPECT_TRUE(validateExpression("", ".*"));
}
TEST(Thread, JoinsAndMove) {
  int value = 0;
  {
    ThreadWrapper worker([&](int n) { value = n; }, 7);
    EXPECT_TRUE(worker.joinable());
    ThreadWrapper moved(std::move(worker));
    EXPECT_FALSE(worker.joinable());
    moved.join();
    EXPECT_FALSE(moved.joinable());
    moved.join();
  }
  EXPECT_EQ(value, 7);
  {
    ThreadWrapper worker([&] { value = 9; });
  }
  EXPECT_EQ(value, 9);
}
TEST(List, OwnershipAndOrder) {
  link_list<int> l;
  l.insert(1).insert(2).insert(3);
  EXPECT_EQ(l.size(), 3u);
  EXPECT_EQ(std::vector<int>(l.begin(), l.end()), (std::vector<int>{3, 2, 1}));
  auto copy = l;
  copy.insert(4);
  EXPECT_EQ(l.size(), 3u);
  std::ostringstream s;
  l.print(s);
  EXPECT_EQ(s.str(), "3\n2\n1\n");
  link_node<int> node(5);
  EXPECT_EQ(node.value, 5);
  EXPECT_EQ(node.next, nullptr);
  EXPECT_EQ(node.prev, nullptr);
}

TEST(Functions, RetrySuccessFailureAndVoid) {
  int calls = 0;
  EXPECT_EQ(retry(3,
                  [&] {
                    if (++calls < 3) throw std::runtime_error("again");
                    return 7;
                  }),
            7);
  EXPECT_EQ(calls, 3);
  calls = 0;
  EXPECT_THROW(retry(2,
                     [&]() -> int {
                       ++calls;
                       throw std::runtime_error("failure");
                     }),
               std::runtime_error);
  EXPECT_EQ(calls, 2);
  EXPECT_THROW(retry(0, [] { return 1; }), std::invalid_argument);
  calls = 0;
  retry(1, [&] { ++calls; });
  EXPECT_EQ(calls, 1);
  calls = 0;
  EXPECT_THROW(retry<std::logic_error>(4,
                                       [&] {
                                         ++calls;
                                         throw std::runtime_error("no retry");
                                       }),
               std::runtime_error);
  EXPECT_EQ(calls, 1);
}
TEST(Functions, MemoizeAndExceptions) {
  int calls = 0;
  auto cached = memoize<int(int, int)>([&](int a, int b) {
    ++calls;
    return a + b;
  });
  EXPECT_EQ(cached(2, 3), 5);
  EXPECT_EQ(cached(2, 3), 5);
  EXPECT_EQ(calls, 1);
  EXPECT_EQ(cached(3, 2), 5);
  EXPECT_EQ(cached.cacheSize(), 2u);
  cached.clear();
  EXPECT_EQ(cached.cacheSize(), 0u);
  EXPECT_EQ(cached(2, 3), 5);
  EXPECT_EQ(calls, 3);
  auto failing = memoize<int(int)>([](int) -> int { throw std::runtime_error("fail"); });
  EXPECT_THROW(failing(1), std::runtime_error);
  EXPECT_EQ(failing.cacheSize(), 0u);
}
TEST(Functions, OverloadedVisitor) {
  auto visitor = Overloaded{[](int x) { return x + 1; },
                            [](const std::string& s) { return static_cast<int>(s.size()); }};
  EXPECT_EQ(visitor(2), 3);
  EXPECT_EQ(visitor(std::string("abc")), 3);
}
