# Agent Customisation

## Agent Configuration Changes

* If you want to customise the default agent which is present on the device, you can go to the [Agent Management Dashboard](https://agents.espressif.com) and create a new agent from template (or from scratch).
* A good starting point is to refer the `agent_config.json` file of the existing examples. (This file does not go in the firmware. It is used to create the agent in the Agents Dashboard.)

## Local Tools Changes

* You can add your own local tools, remove some of the default ones or add another MCP server to boost the capabilities of your agent.
* You also need to handle the new local tools in the example code. Refer [Example Customisation](example_customisation.md#customising-local-tools) for more details.

## Update Agent On the Device

* Once you have created the agent, you need to update the agent ID on the device to start using the new agent.
* You can do this by scanning the QR code of the agent with your phone's camera app. This will launch the ESP RainMaker Home app, and then you can select the device on which you want to update the agent. Check [Update Agent On the Device](setup_guide.md#changing-the-agent-on-the-device) for more details.
* Or, you can manually update the agent ID on the device by running the following command in the serial console:

    ```bash
    set-agent <agent-id>
    ```
