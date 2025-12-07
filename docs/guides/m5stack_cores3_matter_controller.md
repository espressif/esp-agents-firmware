# User Guide

The fastest way to get started is to get the ESP RainMaker app from the respective stores:

<div style="display: flex; align-items: center; gap: 12px;">
  <a href="https://apps.apple.com/us/app/esp-rainmaker/id1497491540">
    <img src="https://developer.apple.com/assets/elements/badges/download-on-the-app-store.svg" width="160">
  </a>

  <a href="https://play.google.com/store/apps/details?id=com.espressif.rainmaker">
    <img src="https://play.google.com/intl/en_us/badges/static/images/badges/en_badge_web_generic.png" width="200">
  </a>
</div>



## Configuring the Device

This is required to connect the device to your home's Wi-Fi network, which is necessary for the device to function.

- Once you have installed and launched the app, the app will ask for Bluetooth/Location permissions. This is required for the app to detect the unconfigured device's advertisement data.
- Sign in with your ESP RainMaker credentials, or create a new account if you don't have one.
- Power-on the device. It will show a QR Code (For devices with no display, the QR Code will be part of the product packaging)

<img src="https://github.com/espressif/esp_agents_firmware/wiki/images/M5MCQRcode.jpg" width="200" style="display:block">

- In the ESP RainMaker app, click on the "+" icon on the top right. With the phone's camera scan the QR code of the device.

<img src="https://github.com/espressif/esp_agents_firmware/wiki/images/RMAddDevices.PNG" width="200" style="display:block">

- Once the app detects the device, it will ask for the Wi-Fi network and its passphrase to be programmed in the device
- During configuration, it will ask you to select group (select "Home" group if available, or create one) for the matter controller.

<img src="https://github.com/espressif/esp_agents_firmware/wiki/images/RMSelectGroup.PNG" width="200" style="display:block">

- After provisioning, it will ask you to re-login as part of the Controller Configuration. (Note: On Android, you may have to tap on the device tile in Home Screem for login)

<img src="https://github.com/espressif/esp_agents_firmware/wiki/images/RMReLogin.PNG" width="200" style="display:block">

- Once successful, the agent with Matter controller will be added to the selected group.

<img src="https://github.com/espressif/esp_agents_firmware/wiki/images/RMDevices.PNG" width="200" style="display:block">

- You can set agent-id, agent name, Thread border, or update device list, etc. from the device screen.

<img src="https://github.com/espressif/esp_agents_firmware/wiki/images/RMAgentController.PNG" width="200" style="display:block">


## Using the Device

- Once the device is configured as per the steps above, it connects to your Wi-Fi network.

<img src="https://github.com/espressif/esp_agents_firmware/wiki/images/M5MCBootUp.jpg" width="200" style="display:block">

- Now you can say "Hi ESP" to wake the device up.
You could also wake the device up by tapping once on the screen of the device.
The device will play a chime, indicating it's ready for conversation.

- The device is in listening mode when it shows the below icon. You may now ask it any questions, and it will respond to you.

<img src="https://github.com/espressif/esp_agents_firmware/wiki/images/M5MCListening.jpg" width="200" style="display:block">

- You can interrupt the agent anytime by touching the screen.

<img src="https://github.com/espressif/esp_agents_firmware/wiki/images/M5MCSpeaking.jpg" width="200" style="display:block">

- You can ask the agent to control the device in the ESP RainMaker group (here is the "Bathroom").

- The device goes back to sleep if you tap on screen while it is in listening mode

## Default Agent

The device has a Matter controller agent out of the box. It has following capabilities:
- Knowing local time at user's location
- Adjusting volume of your device
- Adjusting the emoji on display based on mood of the conversation
- Control your Matter devices

## Reset to Factory

You can factory reset your device using either of following methods:
- Touch the screen and hold for 10 seconds
- Reflash the firmware using [ESP Launchpad](https://espressif.github.io/esp-launchpad/minimal-launchpad/?flashConfigURL=https://raw.githubusercontent.com/espressif/esp-agents-firmware/refs/heads/main/docs/launchpad/matter_controller/m5stack_cores3.toml)
- Go to device settings in ESP RainMaker Home app -> Factory Reset

## Changing the Agent on the Device

To change the agent on the device, you will need the "ESP RainMaker Home" app. Download and install the app from the App Store (iOS) or Google Play (Android):

<div style="display: flex; align-items: center; gap: 12px;">
  <a href="https://apps.apple.com/in/app/esp-rainmaker-home/id1563728960">
    <img src="https://developer.apple.com/assets/elements/badges/download-on-the-app-store.svg" width="160">
  </a>
  <a href="https://play.google.com/store/apps/details?id=com.espressif.novahome">
    <img src="https://play.google.com/intl/en_us/badges/static/images/badges/en_badge_web_generic.png" width="200">
  </a>
</div>


> Note: The User credentials for ESP RainMaker Home app will be the same as the ESP RainMaker app.
> If you are using this app for the first time, your devices will automatically be added to the "Home" group.
> If you had previously used this app, your devices will be added to the "Home" group by default.


When you create a new agent, in the Agents Dashboard you can click on Share Agent and generate the QR Code.

<img src="https://github.com/espressif/esp_agents_firmware/wiki/images/ShareAgent.png" width="200" style="display:block">

Now scan the QR Code from your phone's camera app / Google lens. The app will then launch the ESP RainMaker Home app.

The app will ask you to select the device that needs to be configured with this agent, and then it will go ahead and configure that device.

<img src="https://github.com/espressif/esp_agents_firmware/wiki/images/ShareAgentDevice.jpeg" width="200" style="display:block">

After selecting your device, the app will update the Agent ID on that device.

<img src="https://github.com/espressif/esp_agents_firmware/wiki/images/ShareAgentSuccess.jpeg" width="200" style="display:block">
