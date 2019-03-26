//
// Created by Peyman Mortazavi on 2019-02-11.
//

#include <susperia/internal/networking/routing/graph_router.h>

using namespace std;
using namespace suspiria::networking;


regex create_regex(const string& pattern) {
  return regex{pattern, regex_constants::ECMAScript | regex_constants::optimize};
}


RegexRouteMatcher::RegexRouteMatcher(const string &pattern, vector<string>&& capture_names) {
  this->pattern_ = create_regex(pattern);
  this->names_ = move(capture_names);
}

RegexRouteMatcher::RegexRouteMatcher(const string &pattern, const vector<string>& capture_names) {
  this->pattern_ = create_regex(pattern);
  this->names_ = capture_names;
}

bool RegexRouteMatcher::match(const string &route, RouteParams &params) const noexcept {
  smatch results;
  if (regex_match(route, results, pattern_)) {
    for (unsigned long index = 1; index < results.size(); index++) {
      params[names_[index-1]] = results.str(index);
    }
    return true;
  }
  return false;
}

shared_ptr<RegexRouteMatcher> RegexRouteMatcher::create_from_args(RouteMatcherArgs &&args) {
  if (args.empty()) throw std::invalid_argument("regex pattern missing from the args.");
  auto pattern = args[0];
  args.erase(args.begin());
  return make_shared<RegexRouteMatcher>(pattern, move(args));
}