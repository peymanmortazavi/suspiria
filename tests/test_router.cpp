//
// Created by Peyman Mortazavi on 2019-02-17.
//

#include <memory>

#include <gtest/gtest.h>

#include <susperia/suspiria.h>


using namespace std;
using namespace suspiria::networking;


template<typename T>
void assert_has_handler(const ResolveResult<T>& result, const T& handler) {
  ASSERT_EQ(result.matched, true);
  ASSERT_EQ(*result.handler, handler);
}


class Networking_RouterTests : public ::testing::Test {
protected:
  void SetUp() override {
    router = make_unique<GraphRouter<int>>();
  }

  unique_ptr<GraphRouter<int>> router;
};

TEST_F(Networking_RouterTests, EmptyRouter) {
  auto result = router->resolve("/path/to/resource");
  ASSERT_EQ(result.matched, false);
  ASSERT_EQ(result.handler, nullptr);
  ASSERT_EQ(result.params.empty(), true);

  router->add_route("", make_shared<int>(2));
  assert_has_handler(router->resolve("/"), 2);
}

TEST_F(Networking_RouterTests, SlashVariations) {
  router->add_route("/single/slash/without/trailing/slash", make_shared<int>(1));
  router->add_route("/single/slash/with/trailing/slash/", make_shared<int>(2));
  router->add_route("//////madness///////is///with//us//////here3//", make_shared<int>(3));
  router->add_route("//////madness///////is///with//us//////here4", make_shared<int>(4));
  router->add_route("madness///////is///with//us//////here5", make_shared<int>(5));

  assert_has_handler(router->resolve("/single/slash/without/trailing/slash"), 1);
  assert_has_handler(router->resolve("/single/slash/without/trailing/slash/"), 1);
  assert_has_handler(router->resolve("//single///slash//without//trailing//slash///"), 1);
  assert_has_handler(router->resolve("single///slash//without//trailing//slash///"), 1);
  assert_has_handler(router->resolve("single///slash//without//trailing//slash"), 1);

  assert_has_handler(router->resolve("single///slash//with//trailing//slash"), 2);
  assert_has_handler(router->resolve("/single///slash//with//trailing//slash"), 2);
  assert_has_handler(router->resolve("/single///slash//with//trailing//slash/"), 2);

  assert_has_handler(router->resolve("madness/is/with/us/here3"), 3);
  assert_has_handler(router->resolve("/madness/is/with/us/here3/"), 3);
  assert_has_handler(router->resolve("/madness/is/with/us/here4/"), 4);
  assert_has_handler(router->resolve("/madness/is/with/us/here5/"), 5);
}

TEST_F(Networking_RouterTests, AddRouteWithNode) {
  auto admin_router = make_unique<GraphRouter<int>>();
  admin_router->add_route("/users/", make_shared<int>(24));

  auto conflict_router = make_unique<GraphRouter<int>>();
  conflict_router->add_route("/safe_merge1", make_shared<int>(1));
  conflict_router->add_route("/bad_merge1", make_shared<int>(1));
  conflict_router->add_route("/<:pk>", make_shared<int>(2));

  auto& new_node = router->add_route("/admin/", admin_router->get_root());
  auto info_node = make_shared<RouterNode<int>>();
  info_node->handler = make_shared<int>(25);
  new_node.add_node("info", info_node);
  router->add_route("/conflict/regular_node", make_shared<int>(3));
  router->add_route("/conflict/bad_merge1", make_shared<int>(4));
  router->add_route("/conflict/<re:\\d*>", make_shared<int>(5));
  router->add_route("/conflict/", conflict_router->get_root());

  assert_has_handler(router->resolve("/admin/users/"), 24);
  assert_has_handler(router->resolve("/admin/info/"), 25);
  assert_has_handler(router->resolve("/conflict/safe_merge1/"), 1);
  assert_has_handler(router->resolve("/conflict/bad_merge1/"), 4);
  assert_has_handler(router->resolve("/conflict/regular_node/"), 3);
  assert_has_handler(router->resolve("/conflict/some_random_thing/"), 2);
  assert_has_handler(router->resolve("/conflict/23/"), 5);
}