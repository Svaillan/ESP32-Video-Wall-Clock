# Message System Test Guide

## Overview

The message system now works as follows:

1. **Message Menu State**: Navigate to "SHOW_MESSAGES" using up/down buttons
1. **Waiting Screen**: Shows "waiting for message..." in smallest font with background effects and bounding box
1. **Message Display**: Messages scroll from right to left in size 2 font with proper bounding box
1. **High Priority**: Messages with "high" or "urgent" priority interrupt any menu
1. **Enter Key**: Press Enter while a message is displaying to cancel it and return to previous screen
1. **Queue**: Multiple messages get queued and display one after another

## Testing Commands

### 1. Check Device Status

```bash
curl -i http://192.168.0.107/status
```

### 2. Send Normal Priority Message

```bash
curl -i -X POST http://192.168.0.107/messages \
  -H "Content-Type: application/json" \
  -d '{"text":"Hello from coworker!","priority":"normal"}'
```

### 3. Send High Priority Message (interrupts any menu)

```bash
curl -i -X POST http://192.168.0.107/messages \
  -H "Content-Type: application/json" \
  -d '{"text":"URGENT: Meeting in 5 minutes!","priority":"high"}'
```

### 4. Send Multiple Messages (queue test)

```bash
curl -i -X POST http://192.168.0.107/messages \
  -H "Content-Type: application/json" \
  -d '[
    {"id":"1","text":"First message","priority":"normal"},
    {"id":"2","text":"Second message","priority":"normal"},
    {"id":"3","text":"Third urgent message","priority":"urgent"}
  ]'
```

## Expected Behavior

1. **In Message Menu**: Shows "waiting for message..." until a message arrives
1. **Message Display**:
   - Starts from right edge of screen
   - Scrolls left in size 2 font with black background box
   - Disappears when fully scrolled off left side
   - Returns to "waiting for message..." if more messages in queue
1. **High Priority**: Immediately interrupts any menu and shows message
1. **Enter Key**: Cancels current message and returns to previous screen
1. **Multiple Messages**: Queue and display one after another

## Key Features Implemented

- ✅ HTTP POST message endpoint
- ✅ "waiting for message" in smallest font with bounding box
- ✅ Message queuing system
- ✅ Right-to-left scrolling with size 2 font
- ✅ High priority message interruption
- ✅ Enter key cancellation
- ✅ Proper background effects and bounding boxes
- ✅ Non-blocking message processing
- ✅ Async HTTP request handling

## Serial Monitor

Monitor the device with:

````markdown
# Message System — Quick Test Guide

This file gives concise steps to test the message subsystem (no code changes required).

Overview
--------
- Navigate to the SHOW_MESSAGES state (use Up/Down buttons).
- The screen shows a "waiting for message" placeholder until a message arrives.
- Messages scroll right→left in font size 2 with a full-width background box.
- High/urgent priorities interrupt the current display and are shown immediately.
- Press Select/Enter to cancel the current message and return to the previous screen.
- Messages queue and display one after another.

Basic tests
-----------

1) Check status

```bash
curl -i http://192.168.0.107/status
````

2. Send a normal message

```bash
curl -i -X POST http://192.168.0.107/messages \
  -H "Content-Type: application/json" \
  -d '{"text":"Hello from coworker!","priority":"normal"}'
```

3. Send a high-priority message

```bash
curl -i -X POST http://192.168.0.107/messages \
  -H "Content-Type: application/json" \
  -d '{"text":"URGENT: Meeting in 5 minutes!","priority":"high"}'
```

4. Queue multiple messages

```bash
curl -i -X POST http://192.168.0.107/messages \
  -H "Content-Type: application/json" \
  -d '[{"text":"First","priority":"normal"},{"text":"Second","priority":"normal"},{"text":"Third urgent","priority":"urgent"}]'
```

## Authentication

The message API can be protected with a password. Set it in `credentials/message_config.h`:

```cpp
// Leave empty string "" to disable password protection
#define MESSAGE_API_PASSWORD "your_secure_password"
```

Examples for sending a message with credentials:

- Bearer token (recommended):

```bash
curl -i -X POST http://192.168.0.107/messages \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer your_secure_password" \
  -d '{"text":"Hello with auth","priority":"normal"}'
```

- Query parameter fallback:

```bash
curl -i -X POST "http://192.168.0.107/messages?password=your_secure_password" \
  -H "Content-Type: application/json" \
  -d '{"text":"Hello with auth","priority":"normal"}'
```

## Notes

- If `MESSAGE_API_PASSWORD` is an empty string, authentication is disabled.
- The system enforces rate limiting (default 500ms) and a max message length (default 500 chars).

## Serial monitor

Run the serial monitor and look for MessageClient logs:

```bash
pio device monitor -e esp32dev
```

Expected logs include:

- MessageClient: HTTP server started on port 80
- MessageClient: queued message: ...
- Enqueue message: ...
- Starting message display: ...

```
```
