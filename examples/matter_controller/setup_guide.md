# Setup Guide

Get started by installing the `ESP RainMaker` app from the respective stores:

> Note: We are in the process of adding support for `Matter Controller` in the `ESP RainMaker Home` app. Once this is complete, the app can also be used for setup.

<div style="display: flex; align-items: center; gap: 12px;">
  <a href="https://apps.apple.com/us/app/esp-rainmaker/id1497491540" target="_blank">
    <img src="https://developer.apple.com/assets/elements/badges/download-on-the-app-store.svg" width="160">
  </a>

  <a href="https://play.google.com/store/apps/details?id=com.espressif.rainmaker" target="_blank">
    <img src="https://play.google.com/intl/en_us/badges/static/images/badges/en_badge_web_generic.png" width="200">
  </a>
</div>

## Configuring the Device

The device is initially in unconfigured mode. You will need to set it up to connect to your home's Wi-Fi network, so you can begin using the device.

* Once you have installed and launched the app, the app will ask for Bluetooth/Location permission. This is necessary for the app to detect the unconfigured device's advertisement data.

* Power-on the device, the device will then show a QR Code (For devices with no display, the QR Code will be part of the product packaging)

* In the ESP RainMaker Home app, click on the "+" icon on the top right. With the phone's camera scan the QR code of the device.

<img src="https://github.com/espressif/esp_agents_firmware/wiki/images/RMAddDevices.PNG" width="200" style="display:block">

* Once the app detects the device, it will ask for the Wi-Fi network and its passphrase to be programmed in the device

* During configuration, it will ask you to select group (select "Home" group if available, or create one) for the matter controller.

<img src="https://github.com/espressif/esp_agents_firmware/wiki/images/RMSelectGroup.PNG" width="200" style="display:block">

* After provisioning, it will ask you to re-login as part of the Controller Configuration. (Note: On Android, you may have to tap on the device tile in Home Screen for login)

<img src="https://github.com/espressif/esp_agents_firmware/wiki/images/RMReLogin.PNG" width="200" style="display:block">

* Once successful, the agent with Matter controller will be added to the selected group.

<img src="https://github.com/espressif/esp_agents_firmware/wiki/images/RMDevices.PNG" width="200" style="display:block">

* You can set agent-id, agent name, Thread border, or update device list, etc. from the device screen. (Note: To pair Thread device to RainMaker, you should click `Update Thread Dataset` to setup Thread border first)

<img src="https://github.com/espressif/esp_agents_firmware/wiki/images/RMAgentController.PNG" width="200" style="display:block">

* After adding a new Matter device to this group, click `Update Device List` to notify Matter controller the newly commissioned device.


## Using the Device

* Once the device is in configured mode, it connects to your Wi-Fi network.
* Now you can say `Hi, ESP` to wake the device up. The device will play a chime, indicating it's ready for conversation. (You can also wake the device up by tapping anywhere on the screen.)
* The device is in listening mode. You may now ask it any questions, and it will respond to you.
* You can see the response on screen.
* You can interrupt the agent anytime by touching the screen.
* You can ask the agent to control the device in the same ESP RainMaker group.
* When response playback stops, you can ask your next question.
* Device goes to sleep after 15 seconds of inactivity, you can wake it again by saying `Hi, ESP`.

## Reset to Factory

You can factory reset your device by either of the following methods:

* Touch and hold the top of device for 10 seconds. The screen will indicate that the reset is in has started ad it will ask you to release the touch.
* Go to device settings in ESP RainMaker app -> Factory Reset

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
> If you can't find the device in your ESP RainMaker Home app, add devices to the group which is present in both apps simultaneously.

When you create a new agent, in the Agents Dashboard you can click on Share Agent and generate the QR Code.

<img src="https://github.com/espressif/esp-agents-firmware/wiki/images/ShareAgent.png" width="200" style="display:block">

Now scan the QR Code from your phone's camera app / Google lens. The app will then launch the ESP RainMaker Home app.

The app will ask you to select the device that needs to be configured with this agent, and then it will go ahead and configure that device.

<img src="https://github.com/espressif/esp-agents-firmware/wiki/images/ShareAgentDevice.jpeg" width="200" style="display:block">

After selecting your device, the app will update the Agent ID on that device.

<img src="https://github.com/espressif/esp-agents-firmware/wiki/images/ShareAgentSuccess.jpeg" width="200" style="display:block">
