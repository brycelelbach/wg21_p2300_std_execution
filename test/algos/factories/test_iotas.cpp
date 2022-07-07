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

TEST_CASE("Simple test for iotas", "[factories][iotas]") {
  auto o1 = P0TBD::sequence_connect(P0TBD::iotas(1, 3), expect_void_receiver{}, identity_value_adapt{});
  ex::start(o1);
}

TEST_CASE("Stack overflow test for iotas", "[factories][iotas]") {
  auto o1 = P0TBD::sequence_connect(P0TBD::iotas(1, 3000000), expect_void_receiver{}, identity_value_adapt{});
  ex::start(o1);
}

TEST_CASE("iotas returns a sequence_sender", "[factories][iotas]") {
  using t = decltype(P0TBD::iotas(1, 3));
  static_assert(P0TBD::sequence_sender<t>, "P0TBD::iotas must return a sequence_sender");
  REQUIRE(P0TBD::sequence_sender<t>);
}
