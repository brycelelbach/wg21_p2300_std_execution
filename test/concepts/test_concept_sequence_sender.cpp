/*
 * Copyright (c) Lucian Radu Teodorescu
 *
 * Licensed under the Apache License Version 2.0 with LLVM Exceptions
 * (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 *
 *   https://llvm.org/LICENSE.txt
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <catch2/catch.hpp>
#include <sequence.hpp>
#include <test_common/receivers.hpp>
#include <test_common/type_helpers.hpp>

namespace ex = std::execution;
namespace P0TBD = ex::P0TBD;

struct oper {
  oper() = default;
  oper(oper&&) = delete;
  friend void tag_invoke(ex::start_t, oper&) noexcept {}
};

struct my_sequence_sender0 {
  using completion_signatures =
    ex::completion_signatures<             //
      ex::set_value_t(),                   //
      ex::set_error_t(std::exception_ptr), //
      ex::set_stopped_t()>;

  friend oper tag_invoke(P0TBD::sequence_connect_t, my_sequence_sender0, empty_recv::recv0&& r, identity_value_adapt) { return {}; }
};
TEST_CASE("type w/ proper types, is a sequence_sender", "[concepts][sequence_sender]") {
  REQUIRE(P0TBD::sequence_sender<my_sequence_sender0>);
  REQUIRE(P0TBD::sequence_sender<my_sequence_sender0, empty_env>);
}
TEST_CASE("sequence_sender that produces void values, accepts a void receiver and models sequence_sender_to the given receiver", 
    "[concepts][sequence_sender]") {
  REQUIRE(P0TBD::sequence_sender_to<my_sequence_sender0, empty_recv::recv0, identity_value_adapt>);
}

struct my_sequence_sender_int {
  using completion_signatures =
    ex::completion_signatures<             //
      ex::set_value_t(int),                //
      ex::set_error_t(std::exception_ptr), //
      ex::set_stopped_t()>;

  friend oper tag_invoke(P0TBD::sequence_connect_t, my_sequence_sender_int, empty_recv::recv0&&, identity_value_adapt&&) { return {}; }
};
TEST_CASE("my_sequence_sender_int is a sequence_sender", "[concepts][sequence_sender]") {
  REQUIRE(P0TBD::sequence_sender<my_sequence_sender_int>);
  REQUIRE(P0TBD::sequence_sender<my_sequence_sender_int, empty_env>);
}
TEST_CASE("sequence_sender that produces int values, accepts a void receiver and models sequence_sender_to the given receiver",
    "[concepts][sequence_sender]") {
  REQUIRE(P0TBD::sequence_sender_to<my_sequence_sender_int, empty_recv::recv0, identity_value_adapt>);
}
