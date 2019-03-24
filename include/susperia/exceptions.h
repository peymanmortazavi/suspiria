//
// Created by Peyman Mortazavi on 2019-02-17.
//

#ifndef SUSPIRIA_EXCEPTIONS_H
#define SUSPIRIA_EXCEPTIONS_H

#include <exception>
#include <string>

namespace suspiria {

  /**
   * RegistryExceptions
   */
  class RegistryError : public std::exception {};

  class RegistryNotFound : public RegistryError {
  public:
    explicit RegistryNotFound(std::string resource_name) : resource_name_(std::move(resource_name)) {}

    const char* what() const noexcept override {
      auto message = "No resource with name '" + resource_name_ + "' exists in the registry.";
      return message.c_str();
    }

  private:
    std::string resource_name_;
  };


  class RegistryAlreadyExists : public RegistryError {
  public:
    explicit RegistryAlreadyExists(std::string resource_name) : resource_name_(std::move(resource_name)) {}

    const char* what() const noexcept override {
      auto message = "Resource with name '" + resource_name_ + "' already exists in a registry that does not allow overrides.";
      return message.c_str();
    }

  private:
    std::string resource_name_;
  };

}

#endif //SUSPIRIA_EXCEPTIONS_H
