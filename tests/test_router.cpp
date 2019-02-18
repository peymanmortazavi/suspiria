//
// Created by Peyman Mortazavi on 2019-02-17.
//

#include <gtest/gtest.h>

#include <susperia/router.h>


using namespace suspiria::networking;


class Networking_RouterTests : public ::testing::Test {
protected:
  void SetUp() override {
    router = GraphRouter<int>{};
  }

  GraphRouter<int> router;
};

TEST_F(Networking_RouterTests, EmptyRouter) {
}