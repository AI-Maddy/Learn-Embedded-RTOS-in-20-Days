Day 19 Lesson - RTOS Comparison and Selection
=============================================

Decision Matrix for RTOS Selection
-----------------------------------

+----------------+------------+------------+-----------+-----------+
| Feature        | FreeRTOS   | Zephyr     | ThreadX   | ChibiOS   |
+================+============+============+===========+===========+
| License        | MIT        | Apache 2.0 | Commercial| GPL/Comm  |
+----------------+------------+------------+-----------+-----------+
| Footprint      | 4-10 KB    | 8-15 KB    | 2-10 KB   | 5-12 KB   |
+----------------+------------+------------+-----------+-----------+
| Certification  | Optional   | No         | Yes       | Optional  |
+----------------+------------+------------+-----------+-----------+
| Networking     | Add-on     | Built-in   | Add-on    | HAL       |
+----------------+------------+------------+-----------+-----------+
| Best For       | General    | IoT/BLE    | Safety    | Performan.|
+----------------+------------+------------+-----------+-----------+

Selection Criteria
------------------

1. **Licensing**: Open-source vs. commercial
2. **Safety certification**: DO-178, IEC 61508, ISO 26262
3. **Footprint**: Flash/RAM constraints
4. **Ecosystem**: Middleware, tools, community
5. **Performance**: Context switch time, interrupt latency
6. **Hardware support**: MCU/SoC coverage
7. **Learning curve**: Documentation, examples

Decision Process
----------------

1. List requirements (real-time, safety, connectivity)
2. Evaluate licensing constraints
3. Check hardware support
4. Prototype with top 2-3 candidates
5. Measure performance on target
6. Consider long-term support

Case Studies
------------

- Medical Device: ThreadX (safety-certified)
- IoT Sensor: Zephyr (BLE built-in)
- Motor Controller: FreeRTOS (simple, proven)
- Automotive: ChibiOS/ThreadX (performance/safety)
