//
// Created by Peyman Mortazavi on 2019-02-09.
//

#ifndef SUSPIRIA_ROUTER_H
#define SUSPIRIA_ROUTER_H

#include <map>
#include <memory>
#include <string>


namespace suspiria {

  namespace networking {

    template<class T>
    class Router {
    public:
      void add_handler(const std::string& path, std::shared_ptr<T> handler) {
        this->_handlers[path] = std::move(handler);
      }

      std::shared_ptr<T> resolve(const std::string& path) {
        auto it = this->_handlers.find(path);
        if (it != this->_handlers.end()) {
          return it->second;
        }
        return nullptr;
      }

    protected:
      std::map<std::string, std::shared_ptr<T>> _handlers;
    };

  }

}

#endif //SUSPIRIA_ROUTER_H
