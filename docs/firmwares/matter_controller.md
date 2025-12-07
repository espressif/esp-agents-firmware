# Matter Controller Agent

The firmware for Matter Controller agent supports the following client tools. You may use them in any other agents you wish to create.

- **get_device_list**
  - Description: Get the available Matter Devices in your group
  - Parameters: This tool does not have any parameters

- **control_device**
  - Description: Control a Matter device
  - Parameters:
    - **node_id**
      - Required: yes
      - Type: String
      - Description: Node ID of the device
    - **cluster_id**
      - Required: yes
      - Type: Integer
      - Description: Cluster ID you want to control
    - **command_id**
      - Required: yes
      - Type: Integer
      - Description: Command ID you want to run
    - **command_args**
      - Required: yes
      - Type: String
      - Description: Args of the command in JSON object form. Refer to the [Matter controller doc](https://docs.espressif.com/projects/esp-matter/en/latest/esp32/controller.html#matter-controller)

- **set_volume**
  - Description: Changes the output volume of your Device
  - Parameters:
    - **volume**
      - Required: yes
      - Type: Integer
      - Description: Updated Volume, should be between 0 and 100

- **set_emotion**
  - Description: Changes the emotion displayed during conversation
  - Parameters:
    - **emotion_name**
      - Required: yes
      - Type: String
      - Description: Emotion name. Supported values: Happy, Sad, Wink, Confused, Cry, Angry, Shocked, Sleep, Neutral

- **get_local_time**
  - Description: Informs the LLM about local time and date at your location
  - Parameters: This tool does not have any parameters
