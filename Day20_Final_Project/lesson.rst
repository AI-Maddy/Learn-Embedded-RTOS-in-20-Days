Day 20 Lesson - Final Project
=============================

Capstone Project: Multi-Zone Environmental Monitor
---------------------------------------------------

Build a complete embedded system integrating all RTOS concepts:

System Requirements:
- 4 sensor zones (temp, humidity, pressure, air quality)
- LCD display with touch interface
- SD card data logging
- WiFi telemetry to cloud
- Local control via buttons
- Audio alarms
- Real-time graphing

Tasks:
1. Sensor acquisition tasks (periodic, high priority)
2. Display update task (low priority)
3. Data logging task (medium priority)
4. Network communication task
5. User interface task
6. Alarm monitoring task

Synchronization:
- Mutexes for shared resources (display, SD card, WiFi)
- Queues for data pipelines
- Event groups for alarm conditions
- Semaphores for ISR-task communication

Performance Requirements:
- Sensor sampling: 10 Hz per zone
- Display update: 2 Hz
- Data logging: Continuous
- Network telemetry: 1 Hz
- Button response: < 100ms
- Alarm response: < 50ms

Development Steps:
1. System architecture and task design
2. Schedulability analysis
3. Prototype on development board
4. Integration testing
5. Performance tuning
6. Stress testing
7. Documentation

Success Criteria:
- All deadlines met under stress
- No stack overflows
- Proper error handling
- Professional code quality
- Complete documentation
