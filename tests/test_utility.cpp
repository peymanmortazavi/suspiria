//
// Created by Peyman Mortazavi on 2019-02-15.
//

#include <algorithm>
#include <iostream>
#include <numeric>
#include <vector>
#include <map>

#include <gtest/gtest.h>

#include <susperia/suspiria.h>


using namespace std;
using namespace suspiria;
using namespace suspiria::utility;


TEST(UtilityTests, StringPartitioner) {
  // Empty string
  string_partitioner p{""};
  string word;
  ASSERT_FALSE(p.next(word));

  // Some test cases
  map<tuple<string, char>, vector<string>> test_cases = {
          {{"/path/to/resource", '/'}, {"path", "to", "resource"}},  // With tailing slash.
          {{"/path/to/resource/", '/'}, {"path", "to", "resource"}},  // Without tailing slash.
          {{"peyman.mortazavi", '.'}, {"peyman", "mortazavi"}},  // String doesn't start with separator.
          {{"peyman.mortazavi", '.'}, {"peyman", "mortazavi"}},  // String ends with separator.
          {{"....peyman...mortazavi....", '.'}, {"peyman", "mortazavi"}},  // Multiple separator characters.
  };
  for (auto& test_case : test_cases) {
    vector<string> items;
    const auto& input = get<0>(test_case.first);
    string_partitioner::for_each(input, [&items](auto& item){
      items.push_back(item);
    }, get<1>(test_case.first));
    auto result = accumulate(begin(items), end(items), string{}, [](auto& text, auto& item) { return text + item + " "; });
    ASSERT_TRUE(equal(begin(items), end(items), begin(test_case.second))) << "INPUT: " << input << "\n" << result;
  }

  // Make sure default separator is slash and behaviour of for_each() and next() are similar.
  vector<string> expectation{"api", "v3", "auth"};
  vector<string> items;
  string_partitioner::for_each("api/v3/auth", [&items](auto& item) { items.push_back(item); });
  ASSERT_TRUE(equal(begin(items), end(items), begin(expectation)));

  string_partitioner splitter{"api/v3/auth"};
  word = "";
  for (auto i = 0; splitter.next(word); i++) {
    ASSERT_EQ(word, expectation[i]);
  }
}


TEST(UtilityTests, Registry) {
  registry<int> test_registry;
  test_registry.add("test", make_unique<int>(20));
  ASSERT_EQ(test_registry.get("test"), 20);
  ASSERT_THROW(test_registry.add("test", make_unique<int>(30)), RegistryAlreadyExists);  // override is not ok.
  ASSERT_THROW(test_registry.get("nothing"), RegistryNotFound);
  ASSERT_THROW(test_registry.get("nothing"), RegistryNotFound);  // make sure consecutive calls don't change anything.
  test_registry.add("nothing", make_unique<int>(-1));
  ASSERT_EQ(test_registry.get("nothing"), -1);

  test_registry = registry<int>{true};  // clear out the registry.
  test_registry.add("test", make_unique<int>(10));
  ASSERT_EQ(test_registry.get("test"), 10);
  test_registry.add("test", make_unique<int>(20));  // override should be ok.
  ASSERT_EQ(test_registry.get("test"), 20);
}