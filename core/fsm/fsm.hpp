/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/asio/io_context.hpp>
#include <boost/optional.hpp>
#include <functional>
#include <map>
#include <mutex>
#include <queue>
#include <set>
#include <shared_mutex>
#include <unordered_map>
#include <utility>

#include "common/error_text.hpp"
#include "common/outcome.hpp"

/**
 * The namespace is related to a generic implementation of a finite state
 * machine
 */
namespace fc::fsm {
  struct EnumClassHash {
    template <typename T>
    std::size_t operator()(T t) const {
      return static_cast<std::size_t>(t);
    }
  };

  template <typename F>
  void postWithFlag(boost::asio::io_context &io,
                    std::weak_ptr<bool> flag,
                    F f) {
    io.post([f{std::move(f)}, flag{std::move(flag)}] {
      if (auto _flag{flag.lock()}; _flag && *_flag) {
        f();
      }
    });
  }

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
   * @tparam EventContextType - user-defined struct to parametrize event
   * @tparam Entity - type of entity to be tracked. Required for enabling
   * callbacks on transitions
   */
  template <typename EventEnumType,
            typename EventContextType,
            typename StateEnumType,
            typename Entity = void>
  class Transition final {
   public:
    /// Type alias for callback on state transition if set
    using ActionFunction = std::function<void(
        std::shared_ptr<Entity> /* pointer to tracked entity */,
        EventEnumType /* event that caused state transition */,
        std::shared_ptr<EventContextType> /* event parameters */,
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
        : event_{event} {}

    Transition(Transition &&) noexcept = default;
    Transition(const Transition &) = default;

    ~Transition() = default;

    Transition &operator=(Transition &&) noexcept = default;
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

    bool isFromAny() const {
      return from_any_;
    }

    std::set<StateEnumType> getFrom() const {
      std::set<StateEnumType> from_states;
      for (auto it = transitions_.begin(); it != transitions_.end(); ++it) {
        from_states.insert(it->first);
      }
      return from_states;
    }

    /**
     * Lookups if there is a transition for a given source state.
     *
     * This method is designed for use by FSM class only.
     *
     * @param from_state - transition source state
     * @param entity_ptr - a shared pointer to an entity. Required for being
     * able to apply transition callback over the entity.
     * @return  returns a resulting state if there is a transition rule or
     * boost::none if there is no transition rule for the current state
     */
    boost::optional<StateEnumType> dispatch(
        StateEnumType from_state,
        const std::shared_ptr<EventContextType> &event_ctx_ptr,
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
          transition_action_.get()(
              entity_ptr, event_, event_ctx_ptr, from_state, to_state);
        }
        return to_state;
      }

      auto lookup = transitions_.find(from_state);
      if (transitions_.end() == lookup) {
        return boost::none;
      }
      if (transition_action_) {
        transition_action_.get()(
            entity_ptr, event_, event_ctx_ptr, from_state, lookup->second);
      }
      return lookup->second;
    }

   private:
    EventEnumType event_;
    bool from_any_{false};
    std::unordered_map<StateEnumType, StateEnumType, EnumClassHash>
        transitions_;
    std::set<StateEnumType> intermediary_;
    boost::optional<ActionFunction> transition_action_;
  };

  /**
   * Finite State Machine implementation
   * @tparam EventEnumType - enum class with list of events
   * @tparam EventContextType - user-defined struct to parametrize event
   * @tparam StateEnumType - enum class with list of states
   * @tparam Entity - type of handled objects, actually std::shared_ptr<Entity>
   */
  template <typename EventEnumType,
            typename EventContextType,
            typename StateEnumType,
            typename Entity>
  class FSM : public std::enable_shared_from_this<
                  FSM<EventEnumType, EventContextType, StateEnumType, Entity>> {
   public:
    using EntityPtr = std::shared_ptr<Entity>;
    using EventContextPtr = std::shared_ptr<EventContextType>;
    using TransitionRule =
        Transition<EventEnumType, EventContextType, StateEnumType, Entity>;
    using ParametrizedEvent = std::pair<EventEnumType, EventContextPtr>;
    using EventQueueItem = std::pair<EntityPtr, ParametrizedEvent>;
    using ActionFunction = std::function<void(
        /* pointer to tracked entity */
        std::shared_ptr<Entity>,
        /* event that caused state transition */
        EventEnumType,
        /* pointer to event context to pass event's parameters */
        EventContextPtr,
        /* transition source state */
        StateEnumType,
        /* transition destination state */
        StateEnumType)>;

    FSM(const FSM &) = delete;
    FSM(FSM &&) = delete;

    ~FSM() {
      stop();
    }

    FSM &operator=(const FSM &) = delete;
    FSM &operator=(FSM &&) = delete;

    /**
     * Fabric method to create class instance.
     * @param transition_rules - defines state transitions
     * @param io_context - async queue
     * @param discard_event - discards event if it cannot be applied instantly.
     * If set to false the event will be preserved in event queue.
     * @return class instance
     */
    static outcome::result<std::shared_ptr<FSM>> createFsm(
        std::vector<TransitionRule> transition_rules,
        boost::asio::io_context &io_context,
        bool discard_event) {
      OUTCOME_TRY(validateTransitionRules(transition_rules));

      struct make_unique_enabler : public FSM {
        make_unique_enabler(std::vector<TransitionRule> transition_rules,
                            boost::asio::io_context &io_context,
                            bool discard_event)
            : FSM{std::move(transition_rules), io_context, discard_event} {};
      };

      return std::move(std::make_shared<make_unique_enabler>(
          transition_rules, io_context, discard_event));
    }
    friend class FSM;

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
        return ERROR_TEXT("FSM is tracking the entity's state already");
      }
      states_.emplace(entity_ptr, initial_state);
      return outcome::success();
    }

    /**
     * Forces tracking of entity with a certain state
     * @param entity_ptr - a shared pointer to the entity
     * @param state - desired entity's state
     * @return none when successful
     */
    outcome::result<void> force(const EntityPtr &entity_ptr,
                                StateEnumType state) {
      std::unique_lock lock(states_mutex_);
      auto lookup = states_.find(entity_ptr);
      if (states_.end() == lookup) {
        return ERROR_TEXT("Specified element was not tracked by FSM");
      }
      lookup->second = state;
      return outcome::success();
    }

    /// schedule an event for an object
    outcome::result<void> send(const EntityPtr &entity_ptr,
                               EventEnumType event,
                               const EventContextPtr &event_context) {
      if (!*running_) {
        return ERROR_TEXT("FSM has been stopped. No more events get processed");
      }
      std::lock_guard lock(event_queue_mutex_);
      auto was_empty{event_queue_.empty()};
      event_queue_.emplace(entity_ptr,
                           std::make_pair(event, std::move(event_context)));
      if (was_empty) {
        tickAsync();
      }
      return outcome::success();
    }

    /*
     * outcome::result<void> sendSync(const Entity &object, EventEnumType event,
     * EventContextType event_context)
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
        return ERROR_TEXT("Specified element was not tracked by FSM.");
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
      *running_ = false;
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

    size_t getEventQueueSize() const {
      return event_queue_.size();
    }

   private:
    /**
     * Creates a state machine
     * @param transition_rules - defines state transitions
     * @param io_context - async queue
     * @param discard_event - discards event if it cannot be applied instantly.
     * If set to false the event will be preserved in event queue.
     */
    FSM(std::vector<TransitionRule> transition_rules,
        boost::asio::io_context &io_context,
        bool discard_event)
        : running_{std::make_shared<bool>(true)},
          io_context_{io_context},
          discard_event_(discard_event) {
      initTransitions(std::move(transition_rules));
    }

    /**
     * Validates that there are no ambiguous rules like (where G is an event):
     * A -(G)-> B
     * A -(G)-> C
     * @param transition_rules
     * @return error if rules are not valid
     */
    static outcome::result<void> validateTransitionRules(
        const std::vector<TransitionRule> &transition_rules) {
      // if rule is from any, it must not be redeclared
      std::set<EventEnumType> from_any;
      std::map<EventEnumType, std::set<StateEnumType>> unique_rules;

      for (auto rule : transition_rules) {
        const auto event = rule.eventId();
        const auto from_states = rule.getFrom();

        if (from_any.find(event) != from_any.end()) {
          return ERROR_TEXT(
              "Transition rule is ambiguous. Was previously declared as "
              "fromAny.");
        }
        if (rule.isFromAny() && unique_rules.count(event) != 0) {
          return ERROR_TEXT(
              "Transition rule is ambiguous. Previous declaration conflicts "
              "with current forAny.");
        }
        for (const auto &from : from_states) {
          if (unique_rules.count(event) != 0
              && unique_rules[event].count(from) != 0) {
            return ERROR_TEXT(
                "Transition rule is ambiguous. From state was previously "
                "declared");
          }
        }

        if (rule.isFromAny()) {
          from_any.insert(event);
        } else {
          unique_rules[event].insert(from_states.begin(), from_states.end());
        }
      }

      return outcome::success();
    }

    /// populate transitions map
    void initTransitions(std::vector<TransitionRule> transition_rules) {
      for (auto rule : transition_rules) {
        auto event = rule.eventId();
        transitions_.insert({event, std::move(rule)});
      }
    }

    void tickAsync() {
      postWithFlag(io_context_, running_, [this] { tick(); });
    }

    /// async events processor routine
    void tick() {
      EventQueueItem event_pair;
      {
        std::lock_guard lock(event_queue_mutex_);
        event_pair = event_queue_.front();
        event_queue_.pop();
        if (!event_queue_.empty()) {
          tickAsync();
        }
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
      auto parametrized_event = event_pair.second;
      auto event_to = parametrized_event.first;
      auto event_ctx = parametrized_event.second;
      // at least one rule was applied
      bool applied = false;
      // iterate over all the transitions rules for the event
      auto event_handlers = transitions_.equal_range(event_to);
      for (auto &event_handler = event_handlers.first;
           event_handler != event_handlers.second;
           ++event_handler) {
        auto resulting_state = event_handler->second.dispatch(
            source_state, event_ctx, event_pair.first);
        if (resulting_state) {
          {
            std::unique_lock lock(states_mutex_);
            states_[event_pair.first] = resulting_state.get();
          }
          if (any_change_cb_) {
            any_change_cb_.get()(
                event_pair.first,        // pointer to entity
                event_to,                // trigger event
                std::move(event_ctx),    // event context or params
                source_state,            // source state
                resulting_state.get());  // destination state
          }
          applied = true;
          break;
        }
      }

      if (!applied && !discard_event_) {
        // There were no rule for transition. Put event in queue in case it
        // can be handled when 'from' state is changed.
        std::lock_guard lock(event_queue_mutex_);
        event_queue_.push(event_pair);
      }
    }

    std::shared_ptr<bool> running_;
    boost::asio::io_context &io_context_;

    std::mutex event_queue_mutex_;
    std::queue<EventQueueItem> event_queue_;

    /// a dispatching list of events and what to do on event
    std::multimap<EventEnumType, TransitionRule> transitions_;

    /// a list of entities' current states
    mutable std::shared_mutex states_mutex_;
    // TODO(turuslan): FIL-420 check cache memory usage
    std::unordered_map<EntityPtr, StateEnumType> states_;

    /// optional callback called after any transition
    boost::optional<ActionFunction> any_change_cb_;

    bool discard_event_;
  };
}  // namespace fc::fsm
