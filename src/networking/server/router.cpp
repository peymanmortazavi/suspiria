//
// Created by Peyman Mortazavi on 2019-02-11.
//

#include <susperia/internal/networking/routing/graph_router.h>

using namespace std;
using namespace suspiria::networking;


RegexRouteMatcher::RegexRouteMatcher(const std::string &pattern, std::vector<std::string>&& capture_names) {
  this->pattern_ = regex {pattern, regex_constants::ECMAScript | regex_constants::optimize};
  this->names_ = move(capture_names);
}

RegexRouteMatcher::RegexRouteMatcher(const std::string &pattern, const std::vector<std::string>& capture_names) {
  this->pattern_ = regex {pattern, regex_constants::ECMAScript | regex_constants::optimize};
  this->names_ = capture_names;
}

bool RegexRouteMatcher::match(const string &route, RouterParams &params) const noexcept {
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