Day 04 Exercises - Context Switching
======================================

Exercise 1 - Context Switch Timing Measurement
-----------------------------------------------

Objective
~~~~~~~~~
Measure actual context switch overhead on your target hardware and analyze factors affecting performance.

Tasks
~~~~~
1. Enable DWT cycle counter for precise timing
2. Implement context switch measurement function
3. Compare with and without FPU usage
4. Measure impact of interrupt priority
5. Calculate overhead as percentage of CPU

Exercise 2 - Stack Analysis During Context Switch
--------------------------------------------------

Objective
~~~~~~~~~
Understand what gets saved on stack during context switch and verify stack usage.

Tasks
~~~~~
1. Create tasks with different stack content
2. Dump stack before/after context switch
3. Identify saved register values
4. Calculate actual stack usage per switch
5. Verify overflow detection works

Exercise 3 - Optimizing Context Switch Frequency
-------------------------------------------------

Objective
~~~~~~~~~
Reduce context switch overhead by batching operations.

Tasks
~~~~~
1. Implement high-frequency data acquisition (1kHz)
2. Measure baseline context switch count
3. Implement batching to reduce switches
4. Compare CPU overhead before/after
5. Measure improvement in throughput
