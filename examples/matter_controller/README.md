# Matter Controller Example

This is a firmware that combines voice assistant capabilities with Matter smart home protocol integration. \
You can control Matter compatible smart home appliances using voice.

**Note**: You need to commission this example using the `ESP RainMaker` app for this to work. (We are in the process of adding support for `Matter Controller` in the `ESP RainMaker Home` app. Once this is complete, the app can also be used for setup.)

## Features in the firmware

- Conversations in human-like voice
- Support various tools out of the box:
  - Setting reminders: \
    You can ask your agent to remind you about your tasks and events.
  - Setting volume: \
    You can ask your agent to adjust it's volume.
  - Getting local time: \
    The agent is aware of the time at your time-zone.
  - Setting emotion: \
    The agent can update the emoji on the display based on mood of the conversation.
  - Getting device list: \
    The agent can get the list of Matter devices in the network.
  - Controlling devices: \
    The agent can control the Matter devices in the network.

## Default Agent

`matter_controller` is the default agent for this example.

The Matter Controller agent is capable of controlling local Matter devices in the network.
It uses the tools from the firmware and can do the following tasks:

- Knowing local time at your location
- Setting Reminders
- Adjusting volume of your device
- Updating the emoji on the display based on mood of the conversation
- Getting the list of Matter devices in the network
- Controlling your Matter devices in the network. Supported Matter clusters:
  - OnOff
  - LevelControl
  - ColorControl

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
- `m5stack_cores3_h2_gateway`

#### Running the Matter Controller with Thread Border Router

To run the Matter Controller example with Thread Border Router support, use either the `M5Stack Thread Border Router` board or the `M5Stack CoreS3` board paired with the `H2 Gateway Module`, along with the dedicated board configuration:

```bash
idf.py select-board --board m5stack_cores3_h2_gateway
```

### 2. Build, Flash and Monitor logs

```bash
idf.py build flash monitor
```

### 3. Set up the device

Next is to set up the device. Add the Wi-Fi credentials and finish the agent setup on the device. Refer the [Setup Guide](setup_guide.md) for more details.

## Advanced Configuration

To customise and use your own agent on the device, refer [Agent Customisation](../../docs/agent_customisation.md). You may or may not need to update the firmware on the device, depending on the changes you make to the agent.

To customise the board and add a custom one, refer [Board Customisation](../../docs/board_customisation.md).
