//
// Created by Peyman Mortazavi on 2019-02-13.
//

#include <susperia/utility.h>

using namespace std;
using namespace suspiria::utility;


bool string_partitioner::next(string &word) {
  for(auto index = _start; index < _text.length(); index++) {
    if (_text[index] == _separator) {
      if (_count) {
        word = _text.substr(_start + 1, _count);
        _start = index;
        _count = 0;
        return true;
      }
    } else {
      _count++;
    }
  }
  if (_count) {
    word = _text.substr(_start + 1);  // update the word one last time if there has been any string left.
    _start = _text.length();  // this'll make the next run not enter the loop.
    _count = 0;  // this'll make the next run not come here again.
    return true;
  }
  return false;
}