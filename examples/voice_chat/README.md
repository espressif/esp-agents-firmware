# Voice Chat Example

This is a basic voice chat firmware that allows users to have natural conversations through voice commands. \
It can respond to your commands, have fun conversations, and help you in day-to-day tasks.

## Features in the firmware

- Conversations in human-like voice
- Supported tools:
  - Setting reminders: \
    You can ask your agent to remind you about your tasks and events.
  - Setting volume: \
    You can ask your agent to adjust its volume.
  - Getting local time: \
    The agent is aware of the time at your time-zone.
  - Setting emotion: \
    The agent can update the emoji on the display based on mood of the conversation.

## Default Agent

`friend` is the default agent for this example.

This is your virtual friend, you can chat and have fun conversations about any topic.
It uses the tools from the firmware and can do the following tasks:

- Having fun conversations
- Knowing local time at your location
- Setting Reminders
- Adjusting volume of your device
- Updating the emoji on the display based on mood of the conversation
- Controlling your ESP RainMaker devices

Refer [agent_config.json](agent_config.json) for the default configuration of this agent. (This file does not go in the firmware. It is used to create the agent in the ESP Private Agents Dashboard.)

## Running the Example

### 0. Prerequisites

- ESP-IDF v5.5.2 or later (must be on the `release/5.5` branch)

### 1. Select Board

```bash
idf.py select-board --board <board>
```

Replace `<board>` with one of:

- `echoear_core_board_v1_2`
- `esp_box_3`
- `m5stack_cores3`
<!-- - `m5stack_cores3_h2_gateway` -->

### 2. Build, Flash and Monitor logs

```bash
idf.py build flash monitor
```

### 3. Set up the device

Next is to set up the device. Add the Wi-Fi credentials and finish the agent setup on the device. Refer the [Setup Guide](setup_guide.md) for more details.

## Advanced Configuration

To customise and use your own agent on the device, refer [Agent Customisation](../../docs/agent_customisation.md). You may or may not need to update the firmware on the device, depending on the changes you make to the agent.

To customise the board and add a custom one, refer [Board Customisation](../../docs/board_customisation.md).
