//
// Created by Peyman Mortazavi on 2019-02-11.
//

#include <susperia/router.h>

using namespace std;
using namespace suspiria::networking;


RegexRouteMatcher::RegexRouteMatcher(const vector<string> &args) : RouteMatcher(args) {
  this->_rule = regex {args[0], regex_constants::ECMAScript | regex_constants::optimize};
  copy(begin(args)+1, end(args), back_inserter(this->_names));
}

bool RegexRouteMatcher::match(const string &route, RouterParams &params) const {
  smatch results;
  if (regex_match(route, results, _rule)) {
    for (unsigned long index = 1; index < results.size(); index++) {
      params[_names[index-1]] = results.str(index);
    }
    return true;
  }
  return false;
}