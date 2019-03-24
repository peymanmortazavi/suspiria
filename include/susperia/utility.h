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
      explicit registry(bool allow_overrides=false) : allow_overrides_(allow_overrides) {}

      void add(const std::string& name, std::unique_ptr<T>&& obj) {
        if (!allow_overrides_ && storage_.find(name) != end(storage_)) {
          throw RegistryAlreadyExists{name};
        }
        this->storage_[name] = std::move(obj);
      }

      T& get(const std::string& name) const {
        const auto& it = this->storage_.find(name);
        if (it != end(this->storage_)) {
          return *it->second;
        }
        throw RegistryNotFound{name};
      }

    private:
      bool allow_overrides_;
      std::map<std::string, std::unique_ptr<T>> storage_;
    };


    /**
     * String splitter used to split text.
     */
    class string_partitioner {
    public:
      explicit string_partitioner(const std::string& text, char separator='/') : text_(text), separator_(separator) {}
      bool next(std::string& word);
      template<typename T> static void for_each(const std::string& text, T func, char separator='/') {
        string_partitioner it{text, separator};
        std::string route;
        while (it.next(route)) func(route);
      }

    private:
      const std::string& text_;
      char separator_;
      unsigned long start_ = 0;
      unsigned long count_ = 0;
    };

  }

}

#endif //SUSPIRIA_UTILITY_H
