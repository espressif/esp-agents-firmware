# Generic Agents Firwmare

This generic agents firmware supports the following local tools. You may use them in any other agents you wish to create.

- **set_volume**
  - Description: Changes the output volume of your EchoEar
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
      - Type: string
      - Description: Emotion name. supported values: Happy, Sad, Wink, Confused, Cry, Angry, Shocked, Sleep, Neutral

- **set_reminder**
  - Description: Sets a reminder
  - Parameters:
    - **task**
      - Required: yes
      - Type: string
      - Description: The to be reminded about
    - **timeout**
      - Required: yes
      - Type: Integer
      - Description: The duration(in seconds) after which to remind

- **get_local_time**
  - Description: Informs the LLM about local time and date at your location
  - Parameters: This tool does not have any parameters
