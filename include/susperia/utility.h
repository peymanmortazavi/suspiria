//
// Created by Peyman Mortazavi on 2019-02-12.
//

#ifndef SUSPIRIA_UTILITY_H
#define SUSPIRIA_UTILITY_H

#include <map>
#include <memory>
#include <string>

namespace suspiria {

  namespace utility {

    template<typename T>
    struct registry {
      void add(const std::string& name, std::shared_ptr<T> obj) {
        this->_storage[name] = obj;
      }

      std::shared_ptr<T> get(const std::string& name) const {
        const auto& it = this->_storage.find(name);
        if (it != end(this->_storage)) {
          return it->second;
        }
        return nullptr;
      }

    private:
      std::map<std::string, std::shared_ptr<T>> _storage;
    };


    template<typename Base, typename ...Args>
    struct Maker {
      virtual Base* make(Args && ...args) = 0;
      virtual std::unique_ptr<Base> make_unique(Args && ...args) = 0;
      virtual std::shared_ptr<Base> make_shared(Args && ...args) = 0;
    };

    template <typename T, typename Base, typename ...Args>
    struct TemplateMaker : Maker<Base, Args...> {
      Base* make(Args && ...args) override { return new T(std::forward<Args>(args)...); }
      std::unique_ptr<Base> make_unique(Args && ...args) override { return std::make_unique<T>(std::forward<Args>(args)...); }
      std::shared_ptr<Base> make_shared(Args && ...args) override { return std::make_shared<T>(std::forward<Args>(args)...); }
    };

    template<typename Base, typename ...Args>
    struct factory {
      typedef Maker<Base, Args...> MakerType;

      template<class T>
      void add(const std::string& name) {
        this->_storage[name] = std::make_unique<TemplateMaker<T, Base, Args...>>();
      }

      MakerType& get_factory(const std::string& name) {
        return *this->_storage[name];
      }

    private:
      std::map<std::string, std::unique_ptr<MakerType>> _storage;
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
