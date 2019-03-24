//
// Created by Peyman Mortazavi on 2019-02-13.
//

#include <susperia/utility.h>

using namespace std;
using namespace suspiria::utility;


bool string_partitioner::next(string &word) {
  for(auto index = start_; index < text_.length(); index++) {
    if (text_[index] == separator_) {
      if (count_) {
        word = text_.substr(start_, count_);
        start_ = index;
        count_ = 0;
        return true;
      } else {
        start_ = index + 1;
      }
    } else {
      count_++;
    }
  }
  if (count_) {
    word = text_.substr(start_);  // update the word one last time if there has been any string left.
    start_ = text_.length();  // this'll make the next run not enter the loop.
    count_ = 0;  // this'll make the next run not come here again.
    return true;
  }
  return false;
}