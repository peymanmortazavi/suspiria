//
// Created by Peyman Mortazavi on 2019-02-12.
//

#ifndef SUSPIRIA_UTILITY_H
#define SUSPIRIA_UTILITY_H

#include <map>
#include <memory>
#include <string>

#include "exceptions.h"

namespace suspiria {

  namespace utility {

    template<typename T>
    class registry {
    public:
      explicit registry(bool allow_overrides=false) : _allow_overrides(allow_overrides) {}

      void add(const std::string& name, std::unique_ptr<T>&& obj) {
        if (!_allow_overrides && _storage.find(name) != end(_storage)) {
          throw RegistryAlreadyExists{name};
        }
        this->_storage[name] = std::move(obj);
      }

      T& get(const std::string& name) const {
        const auto& it = this->_storage.find(name);
        if (it != end(this->_storage)) {
          return *it->second;
        }
        throw RegistryNotFound{name};
      }

    private:
      bool _allow_overrides;
      std::map<std::string, std::unique_ptr<T>> _storage;
    };


    /**
     * String splitter used to split text.
     */
    struct string_partitioner {
      explicit string_partitioner(const std::string& text, char separator='/') : _text(text), _separator(separator) {}
      bool next(std::string& word);
      template<typename T> static void for_each(const std::string& text, T func, char separator='/') {
        string_partitioner it{text, separator};
        std::string route;
        while (it.next(route)) func(route);
      }

    private:
      const std::string& _text;
      char _separator;
      unsigned long _start = 0;
      unsigned long _count = 0;
    };

  }

}

#endif //SUSPIRIA_UTILITY_H
