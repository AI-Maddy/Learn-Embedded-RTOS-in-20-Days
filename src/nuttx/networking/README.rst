NuttX Networking Examples
=========================

Purpose
-------
Socket-oriented networking examples demonstrating NuttX's BSD socket API,
TCP/IP stack, and IoT protocols. These examples showcase POSIX-compliant
network programming in embedded RTOS context.

Examples Included
-----------------

**tcp_server.c** (~220 lines)
  - TCP echo server
  - Multi-client support with select()
  - Socket options configuration
  - Error handling and recovery

**tcp_client.c** (~190 lines)
  - TCP client with reconnection
  - connect() with timeout
  - Buffered send/receive
  - Connection state management

**udp_example.c** (~180 lines)
  - UDP send and receive
  - Broadcast and multicast
  - Connectionless communication
  - Datagram handling

**http_client.c** (~250 lines)
  - HTTP GET/POST requests
  - Response parsing
  - Chunked transfer encoding
  - REST API integration

**mqtt_client.c** (~280 lines)
  - MQTT publish/subscribe
  - QoS levels (0, 1, 2)
  - Last Will Testament
  - Automatic reconnection

**telemetry_flow.c** (~240 lines)
  - Sensor data telemetry
  - JSON serialization
  - Periodic transmission
  - Queue-based buffering

Build Instructions
------------------

.. code-block:: bash

   cd nuttx
   ./tools/configure.sh stm32f4discovery:nsh
   make menuconfig
   # Enable: Network Support -> TCP/IP Networking
   make
   
   # Add to apps/examples/
   cd apps/examples
   # Copy networking examples
   cd ../../nuttx
   make

NSH Integration
---------------

Examples register as NSH built-in commands:

.. code-block:: text

   nsh> ifconfig
   nsh> tcp_server &
   nsh> tcp_client 192.168.1.100 8080
   nsh> http_client http://api.example.com/data
   nsh> mqtt_client broker.hivemq.com 1883

Network Configuration
---------------------

Configure network in NSH:

.. code-block:: bash

   nsh> ifconfig eth0 192.168.1.10
   nsh> ifconfig eth0 netmask 255.255.255.0
   nsh> route add default 192.168.1.1

Or statically in defconfig:

.. code-block:: ini

   CONFIG_NET=y
   CONFIG_NET_IPv4=y
   CONFIG_NET_TCP=y
   CONFIG_NET_UDP=y
   CONFIG_NETDEV_STATISTICS=y
   CONFIG_EXAMPLES_TCPECHO=y

Socket Programming
------------------

**TCP Server Pattern**:

.. code-block:: c

   int sockfd = socket(AF_INET, SOCK_STREAM, 0);
   bind(sockfd, (struct sockaddr*)&addr, sizeof(addr));
   listen(sockfd, 5);
   
   while (1) {
       int client = accept(sockfd, NULL, NULL);
       // Handle client in new task or with select()
       recv(client, buffer, size, 0);
       send(client, response, len, 0);
       close(client);
   }

**TCP Client Pattern**:

.. code-block:: c

   int sockfd = socket(AF_INET, SOCK_STREAM, 0);
   if (connect(sockfd, (struct sockaddr*)&server, sizeof(server)) < 0) {
       // Retry with backoff
   }
   send(sockfd, request, len, 0);
   recv(sockfd, response, size, 0);
   close(sockfd);

**UDP Pattern**:

.. code-block:: c

   int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
   sendto(sockfd, data, len, 0, 
          (struct sockaddr*)&dest, sizeof(dest));
   recvfrom(sockfd, buffer, size, 0, 
            (struct sockaddr*)&from, &fromlen);

HTTP Client Example
-------------------

Simple HTTP GET:

.. code-block:: c

   // Connect to server
   connect(sockfd, &server, sizeof(server));
   
   // Send HTTP request
   snprintf(request, sizeof(request),
            "GET /api/data HTTP/1.1\r\n"
            "Host: %s\r\n"
            "Connection: close\r\n\r\n",
            hostname);
   send(sockfd, request, strlen(request), 0);
   
   // Receive response
   while ((n = recv(sockfd, buffer, sizeof(buffer), 0)) > 0) {
       parse_http_response(buffer, n);
   }

MQTT Integration
----------------

MQTT publish example:

.. code-block:: c

   // Connect to MQTT broker
   mqtt_connect(&client, "broker.hivemq.com", 1883);
   
   // Publish message
   mqtt_publish(&client, "sensors/temperature", 
                payload, strlen(payload), QOS_1);
   
   // Subscribe to topic
   mqtt_subscribe(&client, "actuators/+", QOS_0);
   
   // Message callback
   void on_message(const char *topic, const char *payload) {
       printf("Received: %s -> %s\n", topic, payload);
   }

Telemetry Pattern
-----------------

Sensor data to cloud:

.. code-block:: c

   // Sensor task: produce data
   while (1) {
       read_sensors(&data);
       mq_send(telemetry_queue, &data, sizeof(data));
       sleep(1);
   }
   
   // Network task: consume and send
   while (1) {
       mq_receive(telemetry_queue, &data, sizeof(data));
       json_serialize(&data, json_buf);
       http_post_json("http://api.cloud.com/data", json_buf);
   }

Security
--------

SSL/TLS with mbedTLS:

.. code-block:: c

   mbedtls_ssl_context ssl;
   mbedtls_ssl_setup(&ssl, &conf);
   mbedtls_ssl_handshake(&ssl);
   mbedtls_ssl_write(&ssl, data, len);
   mbedtls_ssl_read(&ssl, buffer, size);

Testing
-------

**Tools**:
  - netcat: ``nc -l 8080`` (test server)
  - curl: ``curl http://device-ip/`` (test HTTP)
  - mosquitto: ``mosquitto_sub -t '#'`` (test MQTT)

**Wireshark**:
  Capture packets for debugging:
  
  .. code-block:: bash
  
     wireshark -i eth0 -f "host 192.168.1.10"

Performance
-----------

- TCP throughput: 5-10 Mbps (depends on MCU)
- UDP throughput: 8-15 Mbps
- HTTP request latency: 50-200ms
- MQTT publish latency: 20-100ms
- Concurrent connections: 5-10 (configurable)

Common Issues
-------------

**Connection refused**:
  - Check firewall settings
  - Verify server is listening
  - Check network configuration

**Timeout**:
  - Set socket timeout with setsockopt()
  - Increase CONFIG_NET_TCP_RECVDELAY
  - Check network latency

**Memory**:
  - Increase CONFIG_NET_TCP_READAHEAD_BUFSIZE
  - Tune CONFIG_NET_MAX_LISTENPORTS
  - Monitor with "free" NSH command

Advanced Topics
---------------

- IPv6 dual-stack
- TLS/DTLS secure sockets
- WebSocket protocol
- CoAP for constrained devices
- Network interface bonding
- QoS and traffic shaping

Resources
---------

- NuttX Networking Guide: https://nuttx.apache.org/docs/latest/components/net/
- Socket API Reference: POSIX standard
- mbedTLS Documentation: https://tls.mbed.org/
