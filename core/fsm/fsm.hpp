/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_FSM_FSM_HPP
#define CPP_FILECOIN_CORE_FSM_FSM_HPP

#include <functional>
#include <mutex>
#include <queue>
#include <set>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <utility>

#include <boost/optional.hpp>
#include <libp2p/protocol/common/asio/asio_scheduler.hpp>
#include <libp2p/protocol/common/scheduler.hpp>
#include "common/outcome.hpp"
#include "host/context/host_context.hpp"

#include "fsm/error.hpp"
#include "fsm/type_hashers.hpp"

/**
 * The namespace is related to a generic implementation of a finite state
 * machine
 */
namespace fc::fsm {
  using fc::common::EnumClassHash;
  using libp2p::protocol::Scheduler;
  using Ticks = libp2p::protocol::Scheduler::Ticks;

  const Scheduler::Ticks kSlowModeDelayMs = 100;

  /**
   * Container for state transitions caused by an event
   *
   * Initialization methods of the class could arise exceptions.
   * This is designed behavior due to the nature of further way of use.
   *
   * Initialization has to be done via
   * sequential calling from* and to* methods.
   *
   * @tparam EventEnumType - enum class with events listed
   * @tparam StateEnumType - enum class with states listed
   * @tparam Entity - type of entity to be tracked. Required for enabling
   * callbacks on transitions
   */
  template <typename EventEnumType,
            typename StateEnumType,
            typename Entity = void>
  class Transition final {
   public:
    /// Type alias for callback on state transition if set
    using ActionFunction = std::function<void(
        std::shared_ptr<Entity> /* pointer to tracked entity */,
        EventEnumType /* event that caused state transition */,
        StateEnumType /* transition source state */,
        StateEnumType /* transition destination state */)>;

    /*
     * Initializers
     *
     * Throwing exceptions here is perfectly fine when the initialization is
     * hardcoded. Usually, it is.
     */

    /// Constructs transition map container for the \param event
    explicit Transition(EventEnumType event = {})  // good to avoid {} here
        : event_{event}, from_any_{false} {}

    Transition(Transition &&) = default;
    Transition(const Transition &) = default;
    Transition &operator=(Transition &&) = default;
    Transition &operator=(const Transition &) = default;

    /// Set source state for a transition
    Transition &from(StateEnumType from_state) {
      if (from_any_ or not intermediary_.empty()) {
        throw std::runtime_error(
            "Event transition source state redefinition or destination state "
            "was not set.");
      }
      intermediary_.insert(from_state);
      return *this;
    }

    /// Set a list of source states for a transition
    template <typename... States>
    Transition &fromMany(States... states) {
      if (from_any_ or not intermediary_.empty()) {
        throw std::runtime_error(
            "Event transition source state redefinition or destination state "
            "was not set.");
      }
      (intermediary_.insert(states), ...);
      return *this;
    }

    /// Enable transition from any state
    Transition &fromAny() {
      if (not transitions_.empty()) {
        throw std::runtime_error(
            "Event transition source state redefinition or destination state "
            "was not set.");
      }
      from_any_ = true;
      return *this;
    }

    /// Set destination state of a transition
    Transition &to(StateEnumType to_state) {
      if (from_any_) {
        if (not intermediary_.empty()) {
          throw std::runtime_error(
              "Event transition destination state redefinition.");
        }
        intermediary_.insert(to_state);
        return *this;
      }
      if (intermediary_.empty()) {
        throw std::runtime_error(
            "Event transition source state(s) are not set.");
      }
      for (auto from : intermediary_) {
        if (transitions_.end() != transitions_.find(from)) {
          throw std::runtime_error(
              "Event transition source state redefinition or "
              "destination state was not set.");
        }
        transitions_[from] = to_state;
      }
      intermediary_.clear();
      return *this;
    }

    /// Set destination state of transition equal to the source state
    Transition &toSameState() {
      if (1 != intermediary_.size()) {
        throw std::runtime_error(
            "Exactly one source state has to be set prior to instantiation of "
            "the same state transition.");
      }
      return to(*intermediary_.begin());
    }

    /**
     * Set a callback to be called when state transition happens
     * @param callback - a function that takes four arguments:
     * 1. a shared pointer to an entity
     * 2. event that triggered the transition
     * 3. transition source state
     * 4. transition destination state
     * @return self
     */
    Transition &action(ActionFunction callback) {
      if (transition_action_) {
        throw std::runtime_error("Transition callback is already set.");
      }
      transition_action_ = std::move(callback);
      return *this;
    }

    /*
     * Accessors
     */

    /// Getter for event identifier
    EventEnumType eventId() const {
      return event_;
    }

    /**
     * Lookups if there is a transition for a given source state.
     *
     * This method is designed for use by FSM class only.
     *
     * @param from_state - transition source state
     * @param entity_ptr - a shared pointer to an entity. Required for being
     * able to apply transition callback over the entity.
     * @return  returns a resulting state if there is a transition rule
     */
    boost::optional<StateEnumType> dispatch(
        StateEnumType from_state,
        const std::shared_ptr<Entity> &entity_ptr) const {
      if (from_any_) {
        if (1 != intermediary_.size()) {
          /*
           * This is the only place in the class with potentially bad exception
           * usage.
           *
           * May be needs to be refactored with outcome::result or something
           * else. */
          throw std::runtime_error(
              "Use of semi-initialized event transition rule");
        }
        auto to_state = *intermediary_.begin();
        if (transition_action_) {
          transition_action_.get()(entity_ptr, event_, from_state, to_state);
        }
        return to_state;
      }

      auto lookup = transitions_.find(from_state);
      if (transitions_.end() == lookup) {
        return boost::none;
      }
      if (transition_action_) {
        transition_action_.get()(
            entity_ptr, event_, from_state, lookup->second);
      }
      return lookup->second;
    }

   private:
    EventEnumType event_;
    bool from_any_;
    std::unordered_map<StateEnumType, StateEnumType, EnumClassHash>
        transitions_;
    std::set<StateEnumType> intermediary_;
    boost::optional<ActionFunction> transition_action_;
  };

  /**
   * Finite State Machine implementation
   * @tparam EventEnumType - enum class with list of events
   * @tparam StateEnumType - enum class with list of states
   * @tparam Entity - type of handled objects, actually std::shared_ptr<Entity>
   */
  template <typename EventEnumType, typename StateEnumType, typename Entity>
  class FSM {
   public:
    using EntityPtr = std::shared_ptr<Entity>;
    using TransitionRule = Transition<EventEnumType, StateEnumType, Entity>;
    using EventQueueItem = std::pair<EntityPtr, EventEnumType>;
    using HostContext = std::shared_ptr<fc::host::HostContext>;
    using ActionFunction = std::function<void(
        std::shared_ptr<Entity> /* pointer to tracked entity */,
        EventEnumType /* event that caused state transition */,
        StateEnumType /* transition source state */,
        StateEnumType /* transition destination state */)>;

    /**
     * Creates a state machine
     * @param transition_rules - defines state transitions
     * @param scheduler - libp2p Scheduler for async events processing
     */
    FSM(std::vector<TransitionRule> transition_rules,
        HostContext context,
        Ticks ticks = 50)
        : running_{true},
          delay_{kSlowModeDelayMs},
          host_context_(std::move(context)) {
      initTransitions(std::move(transition_rules));
      scheduler_ = std::make_shared<libp2p::protocol::AsioScheduler>(
          *host_context_->getIoContext(),
          libp2p::protocol::SchedulerConfig{ticks});
      scheduler_handle_ = scheduler_->schedule(1000, [this] { onTimer(); });
    }

    ~FSM() {
      stop();
    }

    /**
     * Initiates tracking of entity with a certain initial state
     * @param entity_ptr - a shared pointer to the entity
     * @param initial_state - desired entity's initial state
     * @return none when successful
     */
    outcome::result<void> begin(const EntityPtr &entity_ptr,
                                StateEnumType initial_state) {
      std::unique_lock lock(states_mutex_);
      auto lookup = states_.find(entity_ptr);
      if (states_.end() != lookup) {
        return FsmError::kEntityAlreadyBeingTracked;
      }
      states_.emplace(entity_ptr, initial_state);
      return outcome::success();
    }

    // schedule an event for an object
    outcome::result<void> send(const EntityPtr &entity_ptr,
                               EventEnumType event) {
      if (not running_) {
        return FsmError::kMachineStopped;
      }
      std::lock_guard lock(event_queue_mutex_);
      event_queue_.emplace(entity_ptr, event);
      return outcome::success();
    }

    /*
     * outcome::result<void> sendSync(const Entity &object, EventEnumType event)
     *
     * The method is non-trivial in implementation and is not used in golang
     * implementation. Only async send method is used there.
     * That was the reason to skip its implementation by now.
     */

    /**
     * Retrieves current state of the entity of interest
     * @param entity_pointer - a shared pointer to the entity
     * @return entity state
     */
    outcome::result<StateEnumType> get(const EntityPtr &entity_pointer) const {
      std::shared_lock lock(states_mutex_);
      auto lookup = states_.find(entity_pointer);
      if (states_.end() == lookup) {
        return FsmError::kEntityNotTracked;
      }
      return lookup->second;
    }

    /**
     * Get all states table
     * @return map where a key is a shared pointer to an entity and value is the
     * state
     */
    std::unordered_map<EntityPtr, StateEnumType> list() const {
      std::shared_lock lock(states_mutex_);
      return states_;
    }

    /// Prevent further events processing
    void stop() {
      running_ = false;
      scheduler_handle_.cancel();
    }

    /// Is events processing still enabled
    bool isRunning() const {
      return running_;
    }

    /**
     * Optional. Sets a callback to call on any state transition.
     *
     * It will be called after a specific for the transition callback (if it was
     * set). Note: The callback will be called ONLY when the transition happens.
     * Please distinguish two cases: transition to the same state and no
     * transition - the callback gets called only in the first case.
     *
     * @param action - a function that takes four arguments:
     * 1. a shared pointer to an entity
     * 2. event that triggered the transition
     * 3. transition source state
     * 4. transition destination state
     */
    void setAnyChangeAction(ActionFunction action) {
      any_change_cb_ = std::move(action);
    }

   private:
    /// populate transitions map
    void initTransitions(std::vector<TransitionRule> transition_rules) {
      for (auto rule : transition_rules) {
        auto event = rule.eventId();
        transitions_[event] = std::move(rule);
      }
    }

    /// async events processor routine
    void onTimer() {
      EventQueueItem event_pair;
      if (not running_) {
        return;
      }
      {
        std::lock_guard lock(event_queue_mutex_);
        if (event_queue_.empty()) {
          scheduler_handle_.reschedule(kSlowModeDelayMs);
          return;
        }
        scheduler_handle_.reschedule(0);
        event_pair = event_queue_.front();
        event_queue_.pop();
      }

      StateEnumType source_state;
      {
        std::shared_lock lock(states_mutex_);
        auto current_state = states_.find(event_pair.first);
        if (states_.end() == current_state) {
          return;  // entity is not tracked
        }
        // copy to prevent invalidation of iterator
        source_state = current_state->second;
      }
      auto event_handler = transitions_.find(event_pair.second);
      if (transitions_.end() == event_handler) {
        return;  // transition from the state by the event is not set
      }
      auto resulting_state =
          event_handler->second.dispatch(source_state, event_pair.first);
      if (resulting_state) {
        {
          std::unique_lock lock(states_mutex_);
          states_[event_pair.first] = resulting_state.get();
        }
        if (any_change_cb_) {
          any_change_cb_.get()(event_pair.first,        // pointer to entity
                               event_pair.second,       // trigger event
                               source_state,            // source state
                               resulting_state.get());  // destination state
        }
      }
    }

    bool running_;            ///< FSM is enabled to process events
    Scheduler::Ticks delay_;  ///< minimum async loop delay

    std::mutex event_queue_mutex_;
    std::queue<EventQueueItem> event_queue_;
    std::shared_ptr<Scheduler> scheduler_;
    Scheduler::Handle scheduler_handle_;
    HostContext host_context_;

    /// a dispatching list of events and what to do on event
    std::unordered_map<EventEnumType, TransitionRule> transitions_;

    /// a list of entities' current states
    mutable std::shared_mutex states_mutex_;
    std::unordered_map<EntityPtr, StateEnumType> states_;

    /// optional callback for any transition
    boost::optional<ActionFunction> any_change_cb_;
  };

}  // namespace fc::fsm

#endif  // CPP_FILECOIN_CORE_FSM_FSM_HPP
