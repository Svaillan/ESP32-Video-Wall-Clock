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

```bash
pio device monitor -e esp32dev
```

Look for these log messages:

- "MessageClient: HTTP server started on port 80"
- "MessageClient: queued message: ..."
- "Enqueue message: ..."
- "Starting message display: ..."
