#include <utility>

//
// Created by Peyman Mortazavi on 2019-02-09.
//

#ifndef SUSPIRIA_ROUTER_H
#define SUSPIRIA_ROUTER_H

#include <algorithm>
#include <unordered_map>
#include <memory>
#include <regex>
#include <string>
#include <vector>
#include <functional>

#include <susperia/internal/utility.h>


namespace suspiria {

  namespace networking {

    typedef std::unordered_map<std::string, std::string> RouterParams;

    template<class T>
    struct ResolveResult {
      bool matched = false;
      std::shared_ptr<T> handler = nullptr;
      RouterParams params;
    };

    template<class T>
    class Router {
    public:
      virtual ~Router() = default;
      virtual ResolveResult<T> resolve(const std::string& path) const = 0;
    };

  }

}

#endif //SUSPIRIA_ROUTER_H
