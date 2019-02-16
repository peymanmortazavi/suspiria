//
// Created by Peyman Mortazavi on 2019-02-15.
//

#include <algorithm>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>
#include <map>

#include <gtest/gtest.h>

#include <susperia/utility.h>


using namespace std;
using namespace suspiria::utility;


vector<string> get_items(const string& input) {
  vector<string> items;
  string_partitioner::for_each(input, [&items](auto& item){ items.push_back(item); });
  return items;
}


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
    string_partitioner::for_each(get<0>(test_case.first), [&items](auto& item){
      items.push_back(item);
    }, get<1>(test_case.first));
    string a = accumulate(begin(items), end(items), string{}, [](auto& a, auto&b) { return a + " " + b; });
    ASSERT_TRUE(equal(begin(items), end(items), begin(test_case.second))) << "INPUT: " << get<0>(test_case.first) << "\n" << a;
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