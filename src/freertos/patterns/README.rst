FreeRTOS Patterns
=================

Purpose
-------
This directory contains reusable architectural patterns and design templates for
real-world FreeRTOS applications. These patterns demonstrate best practices for
common embedded system scenarios, going beyond basic API usage to show complete,
production-quality designs.

Each pattern is a complete, self-contained example that can be adapted for your
specific application needs.

Patterns Included
-----------------

**producer_consumer.c**
  Complete producer-consumer implementation:
  
  - Multiple producer tasks generating data
  - Multiple consumer tasks processing data
  - Queue-based communication with backpressure handling
  - Proper error handling and recovery
  - Performance monitoring and statistics
  - Demonstrates load balancing
  - ~250 lines of code

**state_machine.c**
  Finite state machine in RTOS context:
  
  - Event-driven state machine design
  - Queue for event delivery
  - State transition tables
  - Entry/exit actions for states
  - Timeout handling in states
  - Debug logging of transitions
  - ~280 lines of code

Architectural Patterns
----------------------

**Layered Architecture**:
  Separation between task logic and hardware access

**Event-Driven Design**:
  Tasks react to events rather than polling

**Resource Management**:
  Proper acquisition and release of shared resources

**Error Recovery**:
  Graceful degradation and recovery from errors

**Scalability**:
  Patterns can be extended with more tasks/queues

Build Instructions
------------------

Build individual patterns:

.. code-block:: bash

   cd src/freertos/patterns/producer_consumer
   make BOARD=stm32f4_discovery
   make flash

Or build all patterns:

.. code-block:: bash

   cd src/freertos/patterns
   make all

Producer-Consumer Pattern
-------------------------

**Use Cases**:
  - Sensor data acquisition and processing
  - Network packet handling
  - Audio/video processing pipelines
  - Data logging systems

**Design Features**:
  - Decouples producers from consumers
  - Queue provides buffering
  - Handles rate mismatches
  - Can scale with multiple producers/consumers
  - Backpressure when queue fills

**Performance Tuning**:
  - Queue depth affects latency vs. memory
  - Producer/consumer priorities matter
  - Consider using direct-to-task notifications for 1:1 case

**Example Output**:

.. code-block:: text

   Producer1: Generated item 42
   Producer2: Generated item 73
   Consumer1: Processed item 42 (took 5ms)
   Consumer2: Processed item 73 (took 3ms)
   Stats: Queue utilization 45%, Throughput 200 items/sec

State Machine Pattern
---------------------

**Use Cases**:
  - Protocol implementations
  - User interface logic
  - Device mode management
  - Complex control systems

**Design Features**:
  - Clear state definitions
  - Well-defined transitions
  - Event-driven execution
  - Timeout support
  - State persistence (optional)
  - Debug visibility

**State Machine Structure**:

.. code-block:: c

   typedef enum {
       STATE_INIT,
       STATE_IDLE,
       STATE_ACTIVE,
       STATE_ERROR
   } state_t;
   
   typedef enum {
       EVENT_START,
       EVENT_STOP,
       EVENT_ERROR,
       EVENT_TIMEOUT
   } event_t;
   
   // State handler function
   state_t state_idle_handler(event_t event) {
       switch(event) {
           case EVENT_START:
               return STATE_ACTIVE;
           case EVENT_ERROR:
               return STATE_ERROR;
           default:
               return STATE_IDLE;
       }
   }

**Example Output**:

.. code-block:: text

   [FSM] Transition: INIT -> IDLE
   [FSM] Event: START received
   [FSM] Transition: IDLE -> ACTIVE
   [FSM] State ACTIVE: processing...
   [FSM] Event: STOP received
   [FSM] Transition: ACTIVE -> IDLE

Best Practices
--------------

**Task Design**:
  - Single responsibility per task
  - Clear ownership of resources
  - Well-defined interfaces (queues, semaphores)
  - Document blocking behavior

**Communication**:
  - Prefer queues for data passing
  - Use semaphores for synchronization only
  - Consider direct-to-task notifications for simple cases
  - Design for bounded waiting (use timeouts)

**Error Handling**:
  - Every queue/semaphore operation should check return value
  - Have a recovery strategy for each failure mode
  - Log errors with context
  - Consider watchdog for deadlock detection

**Performance**:
  - Minimize critical sections
  - Avoid priority inversion (use mutexes, not binary semaphores for mutual exclusion)
  - Profile with FreeRTOS+Trace or similar tools
  - Monitor stack high water marks

Testing Strategies
------------------

**Unit Testing**:
  - Test state machine transitions independently
  - Mock hardware dependencies
  - Verify error handling paths

**Integration Testing**:
  - Test with realistic loads
  - Verify timing constraints
  - Stress test with overload conditions

**Debugging Tips**:
  - Use configASSERT() liberally
  - Enable stack overflow checking
  - Log state transitions and events
  - Use task/queue names for visibility

Adaptation Guide
----------------

To adapt patterns for your use case:

1. **Identify the pattern**: Which pattern fits your needs?
2. **Modify data structures**: Change queue item types
3. **Adjust priorities**: Based on your timing requirements
4. **Add error handling**: Specific to your application
5. **Integrate hardware**: Replace stubs with real drivers
6. **Test thoroughly**: Especially edge cases and errors

Common Variations
-----------------

**Producer-Consumer**:
  - Add priority queues
  - Implement work stealing
  - Use memory pools for zero-copy
  - Add flow control mechanisms

**State Machine**:
  - Hierarchical states (nested FSMs)
  - Concurrent state machines
  - State history for back transitions
  - State persistence across resets

Performance Metrics
-------------------

Patterns include built-in performance monitoring:

- Task execution time
- Queue utilization (average depth)
- Event processing latency
- Memory usage
- Context switch count
- Stack high water marks

Next Steps
----------

After understanding patterns:

1. Combine multiple patterns in one application
2. Study **final_project/** for integrated example
3. Read FreeRTOS best practices documentation
4. Profile and optimize for your target hardware
