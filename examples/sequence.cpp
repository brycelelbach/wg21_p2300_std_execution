/*
 * Copyright (c) NVIDIA
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

// Pull in the reference implementation of P2300:
#include <sequence.hpp>

#include "./schedulers/static_thread_pool.hpp"
#include "../test/test_common/type_helpers.hpp"

#include <cstdio>

///////////////////////////////////////////////////////////////////////////////
// Example code:
using namespace std::execution;
using namespace std::execution::P0TBD;
using std::this_thread::sync_wait;

struct noop_receiver : receiver_adaptor<noop_receiver> {
        friend receiver_adaptor<noop_receiver>;
        template <class... _As>
          void set_value(_As&&... ) noexcept {
          }
        void set_error(std::exception_ptr) noexcept {
        }
        void set_stopped() noexcept {
        }
        make_env_t<get_stop_token_t, std::never_stop_token> get_env() const& {
          return make_env<get_stop_token_t>(std::never_stop_token{});
        }
};

int main() {
  auto print_each = iotas(1, 10)
  | then_each([](int v){ printf("%d, ", v); }) 
  | ignore_all()
  | ex::then([](auto&&...){ printf("\n"); });

  check_val_types<type_array<type_array<>>>(print_each);
  check_err_types<type_array<std::exception_ptr>>(print_each);
  check_sends_stopped<false>(print_each);

  sync_wait(print_each);
}