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
#pragma once

#include <execution.hpp>

namespace std::execution::P0TBD {

  /////////////////////////////////////////////////////////////////////////////
  // [execution.senders.sequence_connect]
  namespace __sequence_connect {
    struct sequence_connect_t;

    template <class _Sender, class _Receiver, class _ValueAdapter>
      concept __sequence_connectable_sender_with =
        sender<_Sender, env_of_t<_Receiver>> &&
        __receiver_from<_Receiver, _Sender> &&
        tag_invocable<sequence_connect_t, _Sender, _Receiver, _ValueAdapter>;

    struct sequence_connect_t {
      template <class _Sender, class _Receiver, class _ValueAdapter>
        requires __sequence_connectable_sender_with<_Sender, _Receiver, _ValueAdapter>
      auto operator()(_Sender&& __sndr, _Receiver&& __rcvr, _ValueAdapter&& __adapt) const
        noexcept(nothrow_tag_invocable<sequence_connect_t, _Sender, _Receiver, _ValueAdapter>)
        -> tag_invoke_result_t<sequence_connect_t, _Sender, _Receiver, _ValueAdapter> {
        static_assert(
          operation_state<tag_invoke_result_t<sequence_connect_t, _Sender, _Receiver, _ValueAdapter>>,
          "execution::sequence_connect(sender, receiver, value_adapter) must return a type that "
          "satisfies the operation_state concept");
        return tag_invoke(sequence_connect_t{}, (_Sender&&) __sndr, (_Receiver&&) __rcvr, (_ValueAdapter&&) __adapt);
      }
#if 0
      template <class _Awaitable, class _Receiver>
        requires (!__sequence_connectable_sender_with<_Awaitable, _Receiver>) &&
          __callable<__sequence_connect_awaitable_t, _Awaitable, _Receiver>
      auto operator()(_Awaitable&& __await, _Receiver&& __rcvr) const
        -> __sequence_connect_awaitable::__operation_t<_Receiver> {
        return __sequence_connect_awaitable((_Awaitable&&) __await, (_Receiver&&) __rcvr);
      }
#endif
#if 0
      // This overload is purely for the purposes of debugging why a
      // sender will not connect. Use the __debug_sender function below.
      template <class _Sender, class _Receiver>
        requires (!__sequence_connectable_sender_with<_Sender, _Receiver>) &&
           (!__callable<__sequence_connect_awaitable_t, _Sender, _Receiver>) &&
           tag_invocable<__is_debug_env_t, env_of_t<_Receiver>>
      auto operator()(_Sender&& __sndr, _Receiver&& __rcvr) const
        -> __debug_op_state {
        // This should generate an instantiate backtrace that contains useful
        // debugging information.
        using std::__tag_invoke::tag_invoke;
        return tag_invoke(*this, (_Sender&&) __sndr, (_Receiver&&) __rcvr);
      }
#endif
    };

  } // namespace __connect

  using __sequence_connect::sequence_connect_t;
  inline constexpr __sequence_connect::sequence_connect_t sequence_connect {};

  template <class _Sender, class _Receiver, class _ValueAdaptor>
    using sequence_connect_result_t = __call_result_t<sequence_connect_t, _Sender, _Receiver, _ValueAdaptor>;

  template <class _Sender, class _Receiver, class _ValueAdaptor>
    concept __has_nothrow_sequence_connect =
      noexcept(connect(__declval<_Sender>(), __declval<_Receiver>(), __declval<_ValueAdaptor>()));

  // using __connect::__debug_sequence_sender;

  template <class _SequenceSender, class _Env>
    concept __sequence_sender =
      requires (_SequenceSender&& __seq_sndr, _Env&& __env) {
        { get_completion_signatures((_SequenceSender&&) __seq_sndr, (_Env&&) __env) } ->
          __valid_completion_signatures<_Env>;
      };

  template <class _SequenceSender, class _Env = no_env>
    concept sequence_sender =
      __sender<_SequenceSender, _Env> &&
      __sender<_SequenceSender, no_env> &&
      move_constructible<remove_cvref_t<_SequenceSender>>;

  /////////////////////////////////////////////////////////////////////////////
  // [exec.snd]
    template <class...>
      using __remove_value_t = completion_signatures<>;

    template<class _SequenceSender, class _Receiver>
    using __seq_compl_sigs_t =
      make_completion_signatures<
        _SequenceSender,
        env_of_t<_Receiver>,
        completion_signatures<set_value_t()>,
        __remove_value_t>;

  template <class _SequenceSender, class _Receiver, class _ValueAdaptor>
    concept sequence_sender_to =
      sequence_sender<_SequenceSender, env_of_t<_Receiver>> &&
      receiver_of<_Receiver, __seq_compl_sigs_t<_SequenceSender, _Receiver>> &&
      requires (_SequenceSender&& __seq_sndr, _Receiver&& __rcvr, _ValueAdaptor&& __value_adapt) {
        sequence_connect((_SequenceSender&&) __seq_sndr, (_Receiver&&) __rcvr, (_ValueAdaptor&&) __value_adapt);
      };

    /////////////////////////////////////////////////////////////////////////////
  // [execution.senders.factories]
  namespace __iotas {

    template <class _CPO, class _V>
    using __completion_signatures_ = completion_signatures<_CPO(_V)>;

    template <class _CPO, class _V>
    struct __value_sender {
        using completion_signatures = __completion_signatures_<_CPO, _V>;

        template <class _ReceiverId>
          struct __operation : __immovable {
            using _Receiver = __t<_ReceiverId>;
            _Receiver __rcvr_;
            _V __last_;

            friend void tag_invoke(start_t, __operation& __op_state) noexcept {
              static_assert(__nothrow_callable<_CPO, _Receiver, _V>);
              _CPO{}(std::move(__op_state.__rcvr_), std::move(__op_state.__last_));
            }
          };

        _V __last_;

        template <class _Receiver, class _ValueAdapter>
          requires (copy_constructible<_V>)
        friend auto tag_invoke(connect_t, const __value_sender& __sndr, _Receiver&& __rcvr, const _ValueAdapter& __adapt)
          noexcept(is_nothrow_copy_constructible_v<_V>)
          -> __operation<__x<remove_cvref_t<_Receiver>>> {
          return {{}, (_Receiver&&) __rcvr, ((__value_sender&&) __sndr).__last_};
        }

        template <class _Receiver>
        friend auto tag_invoke(connect_t, __value_sender&& __sndr, _Receiver&& __rcvr)
          noexcept(is_nothrow_move_constructible_v<_V>)
          -> __operation<__x<remove_cvref_t<_Receiver>>> {
          return {{}, (_Receiver&&) __rcvr, ((__value_sender&&) __sndr).__last_};
        }
    };

    template <class _CPO, class _V, class _B>
      struct __sender {
        using completion_signatures = __completion_signatures_<_CPO, _V>;
        template <class _ReceiverId, class _ValueAdaptorId>
          struct __operation : __immovable {
            using _Receiver = __t<_ReceiverId>;
            using _ValueAdaptor = __t<_ValueAdaptorId>;

            struct __value_receiver {
                __operation* __op_state_;

                template<class... _Args>
                friend void tag_invoke(set_value_t, __value_receiver&& __value_rcvr, _Args&&...) noexcept try {
                    auto __op_state = ((__value_receiver&&) __value_rcvr).__op_state_;
                    auto& __value_op = __op_state->__value_op_;
                    __value_op.reset();
                    if (__op_state->__bound_ == __op_state->__next_) {
                      execution::set_value(std::move(__op_state->__rcvr_));
                      return;
                    }
                    auto __adapted_value{__op_state->__adapt_(__value_sender<_CPO, _V>{__op_state->__next_++})};
                    __value_op.emplace(
                        __conv{
                            [&](){
                                return execution::connect(std::move(__adapted_value), __value_receiver{__op_state});
                            }
                        });
                    auto __start_id = __op_state->__start_id_;
                    if (std::thread::id{} == __start_id || __start_id != std::this_thread::get_id()) {
                      execution::start(*__value_op);
                    }
                } catch(...) {
                    execution::set_error(
                        (_Receiver&&) ((__value_receiver&&) __value_rcvr).__op_state_->__rcvr_,
                        current_exception());
                }

                template <__one_of<set_error_t, set_stopped_t> _Tag, class... _Args>
                  requires __callable<_Tag, _Receiver, _Args...>
                friend void tag_invoke(_Tag, __value_receiver&& __value_rcvr, _Args&&... __args) noexcept {
                  auto __op_state = ((__value_receiver&&) __value_rcvr).__op_state_;
                  __op_state->__value_op_.reset();
                  _Tag{}((_Receiver&&) __op_state->__rcvr_, (_Args&&) __args...);
                }

                friend auto tag_invoke(get_env_t, const __value_receiver& __value_rcvr)
                  -> env_of_t<_Receiver> {
                  return get_env(__value_rcvr.__op_state_->__rcvr_);
                }
            };

            _Receiver __rcvr_;
            _ValueAdaptor __adapt_;
            [[no_unique_address]] _V __next_;
            [[no_unique_address]] _B __bound_;

            using __value_sender_t=invoke_result_t<_ValueAdaptor, __value_sender<_CPO, _V>>;
            using __value_op_t=connect_result_t<__value_sender_t, __value_receiver>;
            optional<__value_op_t> __value_op_;
            std::thread::id __start_id_;

            friend void tag_invoke(start_t, __operation& __op_state) noexcept {
              static_assert(__nothrow_callable<_CPO, _Receiver, _V>);
              if (__op_state.__bound_ == __op_state.__next_) {
                execution::set_value(std::move(__op_state.__rcvr_));
                return;
              }
              auto __adapted_value{__op_state.__adapt_(__value_sender<_CPO, _V>{__op_state.__next_++})};
              __op_state.__value_op_.emplace(
                  __conv{
                      [&](){
                          return execution::connect(std::move(__adapted_value), __value_receiver{&__op_state});
                      }
                  });
              __op_state.__start_id_ = std::this_thread::get_id();
              while(!!__op_state.__value_op_) {
                execution::start(*__op_state.__value_op_);
              }
              __op_state.__start_id_ = std::thread::id{};
            }
          };

        [[no_unique_address]] _V __value_;
        [[no_unique_address]] _B __bound_;

        template <class _Receiver, class _ValueAdaptor>
        friend auto tag_invoke(sequence_connect_t, const __sender& __sndr, _Receiver&& __rcvr, const _ValueAdaptor& __adapt)
          noexcept(is_nothrow_copy_constructible_v<_V> && is_nothrow_copy_constructible_v<_B>)
          -> __operation<__x<remove_cvref_t<_Receiver>>, __x<remove_cvref_t<_ValueAdaptor>>> {
          return {{}, (_Receiver&&) __rcvr, (_ValueAdaptor&&) __adapt, ((__sender&&) __sndr).__value_, ((__sender&&) __sndr).__bound_};
        }

        template <class _Receiver, class _ValueAdapter>
        friend auto tag_invoke(sequence_connect_t, __sender&& __sndr, _Receiver&& __rcvr, _ValueAdapter&& __adapt)
          noexcept(is_nothrow_copy_constructible_v<_V> && is_nothrow_copy_constructible_v<_B>)
          -> __operation<__x<remove_cvref_t<_Receiver>>, __x<remove_cvref_t<_ValueAdapter>>> {
          return {{}, (_Receiver&&) __rcvr, (_ValueAdapter&&) __adapt, ((__sender&&) __sndr).__value_, ((__sender&&) __sndr).__bound_};
        }
      };

    inline constexpr struct __iotas_t {
      template <copy_constructible _V, __equality_comparable_with<_V> _B>
      __sender<set_value_t, decay_t<_V>, decay_t<_B>> operator()(_V&& __v, _B&& __b) const
        noexcept(is_nothrow_constructible_v<decay_t<_V>, _V> && is_nothrow_constructible_v<decay_t<_B>, _B>) {
        return {(_V&&) __v, (_B&&) __b};
      }
    } iotas {};
  }
  using __iotas::iotas;

  /////////////////////////////////////////////////////////////////////////////
  // [execution.senders.adaptors.ignore_all]
  namespace __ignore_all {

    template <class _SequenceSenderId>
      struct __sequence_sender {
        using _SequenceSender = __t<_SequenceSenderId>;

        [[no_unique_address]] _SequenceSender __seq_sndr_;

        struct __identity_value_adapt {
          template<class _Sender>
          _Sender operator()(_Sender&& __sndr){ return (_Sender&&) __sndr;}
        };

        template <__decays_to<__sequence_sender> _Self, receiver _Receiver>
          requires sequence_sender_to<__member_t<_Self, _SequenceSender>, _Receiver, __identity_value_adapt>
        friend auto tag_invoke(connect_t, _Self&& __self, _Receiver&& __rcvr)
          noexcept(__has_nothrow_sequence_connect<__member_t<_Self, _SequenceSender>, _Receiver, __identity_value_adapt>)
          //recursive template instantiation
          {//-> sequence_connect_result_t<__member_t<_Self, _SequenceSender>, _Receiver, __identity_value_adapt> {
          return P0TBD::sequence_connect(((_Self&&) __self).__seq_sndr_, (_Receiver&&) __rcvr, __identity_value_adapt{});
        }

        template <__decays_to<__sequence_sender> _Self, receiver _Receiver, class _ValueAdaptor>
          requires sequence_sender_to<__member_t<_Self, _SequenceSender>, _Receiver, __identity_value_adapt>
        friend auto tag_invoke(sequence_connect_t, _Self&& __self, _Receiver&& __rcvr, _ValueAdaptor&&)
          noexcept(__has_nothrow_sequence_connect<__member_t<_Self, _SequenceSender>, _Receiver, __identity_value_adapt>)
          //recursive template instantiation
          {//-> sequence_connect_result_t<__member_t<_Self, _SequenceSender>, _Receiver, __identity_value_adapt> {
          return P0TBD::sequence_connect(((_Self&&) __self).__seq_sndr_, (_Receiver&&) __rcvr, __identity_value_adapt{});
        }

        template <__decays_to<__sequence_sender> _Self, class _Env>
        friend auto tag_invoke(get_completion_signatures_t, _Self&&, _Env) ->
          completion_signatures_of_t<_SequenceSender, _Env>;

        // forward sender queries:
        template <__sender_queries::__sender_query _Tag, class... _As>
          requires __callable<_Tag, const _SequenceSender&, _As...>
        friend auto tag_invoke(_Tag __tag, const __sequence_sender& __self, _As&&... __as)
          noexcept(__nothrow_callable<_Tag, const _SequenceSender&, _As...>)
          -> __call_result_if_t<__sender_queries::__sender_query<_Tag>, _Tag, const _SequenceSender&, _As...> {
          return ((_Tag&&) __tag)(__self.__seq_sndr_, (_As&&) __as...);
        }
      };

    struct ignore_all_t {
      template <class _SequenceSender>
        using __sequence_sender = __sequence_sender<__x<remove_cvref_t<_SequenceSender>>>;

      template <sender _SequenceSender>
        requires __tag_invocable_with_completion_scheduler<ignore_all_t, set_value_t, _SequenceSender>
      sequence_sender auto operator()(_SequenceSender&& __seq_sndr) const
        noexcept(nothrow_tag_invocable<ignore_all_t, __completion_scheduler_for<_SequenceSender, set_value_t>, _SequenceSender>) {
        auto __sched = get_completion_scheduler<set_value_t>(__seq_sndr);
        return tag_invoke(ignore_all_t{}, std::move(__sched), (_SequenceSender&&) __seq_sndr);
      }
      template <sender _SequenceSender>
        requires (!__tag_invocable_with_completion_scheduler<ignore_all_t, set_value_t, _SequenceSender>) &&
          tag_invocable<ignore_all_t, _SequenceSender>
      sequence_sender auto operator()(_SequenceSender&& __seq_sndr) const
        noexcept(nothrow_tag_invocable<ignore_all_t, _SequenceSender>) {
        return tag_invoke(ignore_all_t{}, (_SequenceSender&&) __seq_sndr);
      }
      template <sender _SequenceSender>
        requires
          (!__tag_invocable_with_completion_scheduler<ignore_all_t, set_value_t, _SequenceSender>) &&
          (!tag_invocable<ignore_all_t, _SequenceSender>) &&
          sequence_sender<__sequence_sender<_SequenceSender>>
      __sequence_sender<_SequenceSender> operator()(_SequenceSender&& __seq_sndr) const {
        return __sequence_sender<_SequenceSender>{(_SequenceSender&&) __seq_sndr};
      }
      __binder_back<ignore_all_t> operator()() const {
        return {{}, {}};
      }
    };
  }
  using __ignore_all::ignore_all_t;
  inline constexpr ignore_all_t ignore_all{};

  /////////////////////////////////////////////////////////////////////////////
  // [execution.senders.adaptors.then_each]
  namespace __then_each {

    template <class _SequenceSenderId, class _FunId>
      struct __sequence_sender {
        using _SequenceSender = __t<_SequenceSenderId>;
        using _Fun = __t<_FunId>;

        [[no_unique_address]] _SequenceSender __seq_sndr_;
        [[no_unique_address]] _Fun __fun_;

        template<class _ValueAdaptorId>
        struct __each_adapt {
          using _ValueAdaptor = __t<_ValueAdaptorId>;
          [[no_unique_address]] _Fun __fun_;
          [[no_unique_address]] _ValueAdaptor __value_adapt_;
          template<class _Sender>
          auto operator()(_Sender&& __sndr) { return __value_adapt_(execution::then((_Sender&&) __sndr, __fun_)); } //__then_adapt<_ValueAdaptorId>{this})); }
        };

        template <class _ValueAdaptor>
          using __each_adapt_t = __each_adapt<__x<remove_cvref_t<_ValueAdaptor>>>;

        template <class... _Args>
            requires invocable<_Fun, _Args...>
          using __set_value =
            completion_signatures<
              __minvoke1<
                __remove<void, __qf<set_value_t>>,
                invoke_result_t<_Fun, _Args...>>>;

        template <class _Self, class _Env>
          using __completion_signatures =
            make_completion_signatures<
              __member_t<_Self, _SequenceSender>, _Env, __with_exception_ptr, __set_value>;

        template <__decays_to<__sequence_sender> _Self, receiver _Receiver, class _ValueAdaptor>
          requires sequence_sender_to<__member_t<_Self, _SequenceSender>, _Receiver, __each_adapt_t<_ValueAdaptor>>
        friend auto tag_invoke(sequence_connect_t, _Self&& __self, _Receiver&& __rcvr, _ValueAdaptor&& __value_adapt)
          noexcept(__has_nothrow_sequence_connect<__member_t<_Self, _SequenceSender>, _Receiver, __each_adapt_t<_ValueAdaptor>>)
          // recursive template instantiation
          {//-> sequence_connect_result_t<__member_t<_Self, _SequenceSender>, _Receiver, __each_adapt_t<_ValueAdaptor>> {
          return P0TBD::sequence_connect(
              ((_Self&&) __self).__seq_sndr_,
              (_Receiver&&) __rcvr,
              __each_adapt_t<_ValueAdaptor>{((_Self&&) __self).__fun_, (_ValueAdaptor&&) __value_adapt});
        }

        template <__decays_to<__sequence_sender> _Self, class _Env>
        friend auto tag_invoke(get_completion_signatures_t, _Self&&, _Env) ->
          __completion_signatures<_Self, _Env>;

        // forward sender queries:
        template <__sender_queries::__sender_query _Tag, class... _As>
          requires __callable<_Tag, const _SequenceSender&, _As...>
        friend auto tag_invoke(_Tag __tag, const __sequence_sender& __self, _As&&... __as)
          noexcept(__nothrow_callable<_Tag, const _SequenceSender&, _As...>)
          -> __call_result_if_t<__sender_queries::__sender_query<_Tag>, _Tag, const _SequenceSender&, _As...> {
          return ((_Tag&&) __tag)(__self.__seq_sndr_, (_As&&) __as...);
        }
      };

    struct then_each_t {
      template <class _SequenceSender, class _Fun>
        using __sequence_sender = __sequence_sender<__x<remove_cvref_t<_SequenceSender>>, __x<remove_cvref_t<_Fun>>>;

      template <sender _SequenceSender, __movable_value _Fun>
        requires __tag_invocable_with_completion_scheduler<then_each_t, set_value_t, _SequenceSender, _Fun>
      sequence_sender auto operator()(_SequenceSender&& __seq_sndr, _Fun __fun) const
        noexcept(nothrow_tag_invocable<then_each_t, __completion_scheduler_for<_SequenceSender, set_value_t>, _SequenceSender, _Fun>) {
        auto __sched = get_completion_scheduler<set_value_t>(__seq_sndr);
        return tag_invoke(then_each_t{}, std::move(__sched), (_SequenceSender&&) __seq_sndr, (_Fun&&) __fun);
      }
      template <sender _SequenceSender, __movable_value _Fun>
        requires (!__tag_invocable_with_completion_scheduler<then_each_t, set_value_t, _SequenceSender, _Fun>) &&
          tag_invocable<then_each_t, _SequenceSender, _Fun>
      sequence_sender auto operator()(_SequenceSender&& __seq_sndr, _Fun __fun) const
        noexcept(nothrow_tag_invocable<then_each_t, _SequenceSender, _Fun>) {
        return tag_invoke(then_each_t{}, (_SequenceSender&&) __seq_sndr, (_Fun&&) __fun);
      }
      template <sender _SequenceSender, __movable_value _Fun>
        requires
          (!__tag_invocable_with_completion_scheduler<then_each_t, set_value_t, _SequenceSender, _Fun>) &&
          (!tag_invocable<then_each_t, _SequenceSender, _Fun>) &&
          sequence_sender<__sequence_sender<_SequenceSender, _Fun>>
      __sequence_sender<_SequenceSender, _Fun> operator()(_SequenceSender&& __seq_sndr, _Fun __fun) const {
        return __sequence_sender<_SequenceSender, _Fun>{(_SequenceSender&&) __seq_sndr, (_Fun&&) __fun};
      }
      template <class _Fun>
      __binder_back<then_each_t, _Fun> operator()(_Fun __fun) const {
        return {{}, {}, {(_Fun&&) __fun}};
      }
    };
  }
  using __then_each::then_each_t;
  inline constexpr then_each_t then_each{};

} // namespace std::execution::P0TBD
