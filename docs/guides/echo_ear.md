# User Guide

The fastest way to get started is to get the ESP RainMaker Home app from the respective stores:

<div style="display: flex; align-items: center; gap: 12px;">
  <a href="https://apps.apple.com/in/app/esp-rainmaker-home/id1563728960">
    <img src="https://developer.apple.com/assets/elements/badges/download-on-the-app-store.svg" width="160">
  </a>

  <a href="https://play.google.com/store/apps/details?id=com.espressif.novahome">
    <img src="https://play.google.com/intl/en_us/badges/static/images/badges/en_badge_web_generic.png" width="200">
  </a>
</div>

## Configuring the Device
The device is initially in unconfigured mode. You will set it up to connect to your home's Wi-Fi network, so you can begin using the device.

* Once you have installed and launched the app, the app will ask for Bluetooth/Location permission. This is necessary for the app to detect the unconfigured device's advertisement data.

* Power-on the device, the device will then show a QR Code (For devices with no display, the QR Code will be part of the product packaging)

<img src="https://github.com/espressif/esp_agents_firmware/wiki/images/EchoEarProvisioning.jpeg" width="200">


* In the ESP RainMaker Home app, click on the "+" icon on the top right, and select 'Scan QR Code'. With the phone's camera scan the QR code of the device.

<img src="https://github.com/espressif/esp_agents_firmware/wiki/images/AddDevices.jpeg" width="200">

* Once the app detects the device, it will ask for the Wi-Fi network and its passphrase to be programmed in the device

* After the device is configured, the device is ready to be used.

<img src="https://github.com/espressif/esp_agents_firmware/wiki/images/SelectWiFi.jpeg" width="200">


## Using the Device
Once the device is in configured mode, it connects to your Wi-Fi network. You will hear a chime, and see eyes flashing like this

<img src="https://github.com/espressif/esp_agents_firmware/wiki/images/EchoEarBootUp.jpeg" width="200">

Now you can say "Hi ESP" to wake the device up. You could also wake the device up by gently tapping once on the top of the device.

<img src="https://github.com/espressif/esp_agents_firmware/wiki/images/EchoEarListening.jpeg" width="200">

The device is in listening mode when it shows the following display. You may now ask it any questions, and it will respond to you.

<img src="https://github.com/espressif/esp_agents_firmware/wiki/images/EchoEarSpeaking.jpeg" width="200">

## Reset to Factory
If you wish to reset the device to factory, touch and hold the top of the device for 10 seconds.


## Changing the Agent on the Device
When you create a new agent, in the Agents Dashboard you can click on Share Agent and generate the QR Code. 

<img src="https://github.com/espressif/esp_agents_firmware/wiki/images/ShareAgent.png" width="200">

Now scan the QR Code from your phone's camera app. The app will then launch the ESP RainMaker Home app. The app will ask you select the device to which this agent needs to be configured, and configure that device with this agent.
