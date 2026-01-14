# Example Customisation

## Customising local tools

You can add your own tool implementation in the [app_tools.c](../examples/voice_chat/main/app_tools.c) file of your example.
Refer [app_common_tools.c](../examples/common/app_common/src/app_common_tools.c) for reference implementation of local tools. \

Once done, you will also need to add this new tool's configuration to agent. \
Refer [Agent Customisation](agent_customisation.md) for more details.

## Device Manual URL

The device manual URL is used to show the device manual in the ESP RainMaker Home app after the device has been set up.

This is set in the `board_defs.h` file.

## Default Agent ID

You can change the default agent ID by setting the `CONFIG_AGENT_SETUP_DEFAULT_AGENT_ID` in menuconfig.

## Devices with no display

In the `app_main.c` file, comment out the call to `app_display_init()`.

Additionally, also comment out the `app_display_*` functions in the `app_main.c` file.
