Table of Contents
=================

* [What is Matter?](#what-is-matter)
* [Third Reality Matter Projects](#third-reality-matter-projects)
* [Hardware Scheme](#hardware-scheme)
* [Development Environment](#development-environment)
   * [Install Dependencies](#install-dependencies)
   * [Build App Example](#build-app-example)
   * [Compiled Results](#compiled-results)
* [Download Image](#download-image)
   * [Burn BL706](#burn-bl706)
   * [Firmware for Download](#firmware-for-download)
* [Control the Night Light](#control-the-night-light)
   * [Use iPhone and HomePod mini](#use-iphone-and-homepod-mini)
      * [Prepare](#prepare)
      * [Bind iPhone and HomePod](#bind-iphone-and-homepod)
      * [Add a Night Light in Home](#add-a-night-light-in-home)
      * [Reset Apple HomePod](#reset-apple-homepod)
      * [Remove Accessories from Home](#remove-accessories-from-home)
   * [Use Android Phone and Google Nest](#use-android-phone-and-google-nest)
      * [Prepare](#prepare-1)
      * [Set up Google Nest](#set-up-google-nest)
      * [Add a Night Light in Home](#add-a-night-light-in-home-1)
      * [Reset Google Nest](#reset-google-nest)
   * [Use Android Phone and Echo V4](#use-android-phone-and-echo-v4)
   * [Factory Reset Night Light](#factory-reset-night-light)
* [Questions and Answers](#questions-and-answers)



# What is Matter?

Matter is a unified, open-source application-layer connectivity standard built
to enable developers and device manufacturers to connect and build reliable, and
secure ecosystems and increase compatibility among connected home devices. It is
built with market-proven technologies using Internet Protocol (IP) and is
compatible with Thread and Wi-Fi network transports. Matter was developed by a
Working Group within the Connectivity Standards Alliance (Alliance). This
Working Group develops and promotes the adoption of the Matter standard, a
royalty-free connectivity standard to increase compatibility among smart home
products, with security as a fundamental design tenet. The vision that led major
industry players to come together to build Matter is that smart connectivity
should be simple, reliable, and interoperable.



# Third Reality Matter Projects

Third Reality  has actively participated in Matter. By the end of 2024, it has just developed 
many products, (two night light, relay module, Home Assistant, Smart Bridge, Smart Plug, etc.) 
some of which providing open source code and hardware for interested developers 
to download code, compile and burn firmware to the light for testing.
Users can voice control lights (switch lights, adjust colors, and brightness) 
on HomePod mini, HomePod, Google Nest, Amazon Echo and other speakers that 
support Matter. Needless to say, users can also control these lights through their 
mobile phones.

This lamp has motion sensor and light sensor inside. With the help of 
these sensors, developers can directly develop practical night light lighting products. 
Third Reality  will open source all relevant code at an appropriate time to facilitate 
developers. 

Third Reality keeps updating the software and hardware of the existing Matter 
products, and provides users with more reference examples (in fact, it is a product with 
complete functions, and it will be easier for users to modify the code on this basis, 
and can achieve full customization functions).  At the same time, develops new products 
that support Matter standard.

Third Reality will pay attention to the progress of the Matter and upgrade with it 
synchronously, so that users can experience the latest version of Matter earlier. 
Matter is as vibrant as the Amazon rainforest. Let's go ahead.



# Hardware Scheme
<img src="./docs_3r/System_Block_Diagram.jpg">

From this path: `"./docs_3r/hardware"`, you can download the hardware implementation schematic, refer to the scheme, and conduct in-depth research against the code.



# Development Environment
The following steps in this document were validated on Ubuntu 20.04.

## Install Dependencies

- Install dependencies as specified in the **connectedhomeip** repository: [Building Matter](https://github.com/project-chip/connectedhomeip/blob/master/docs/guides/BUILDING.md).

- Clone and initialize the **connectedhomeip** repo

  `git clone https://github.com/thirdreality/thread_nightlight.git`

  `cd thread_nightlight`

  `git submodule update --init --recursive`

  `source ./scripts/activate.sh -p bouffalolab`

- Setup build environment for `Bouffalo Lab` SoC

  `./integrations/docker/images/stage-2/chip-build-bouffalolab/setup.sh`

  Script `setpu.sh` requires to select install path, and please execute following command to export `BOUFFALOLAB_SDK_ROOT` before building.

  `export BOUFFALOLAB_SDK_ROOT="Your install path"`

  Suggest using this path /opt/bouffalolab_sdk

## Build App Example

​	`./3r_build.sh`

## Compiled Results

​	out/bouffalolab-bl706-night-light-light-easyflash/chip-bl702-lighting-example.bin


# Download image

The burning tool ["**Bouffalo Lab Dev Cube**"](https://dev.bouffalolab.com/download) can be downloaded from the bouffalo official website 


## Burn BL706

1. prepare a USB cable with one female end and one male end
2. connect the Night Light with a female USB port
3. press and hold the key on the Night Light’s pinhole
4. connect the male USB port to the USB interface of the computer
5. then release the Night Light’s key to enter the burning mode
6. run BLDevCube.exe ( at burning tool directory ) on Windows OS 
7. Set parameters as shown in the following screenshot (get partition table and firmware from the build, dts from the burning tool)
8. click Create & Download button
9. wait until the progress bar is 100%, which means the burning is completed

<img src="./docs_3r/image-20221231151940933.png">


## Firmware for Download

From this path: `"./docs_3r/firmware"`, you can download the compiled nightlight bin and
the dependent firmware, and directly burn and test them without compiling them yourself.



# Control the Night Light



## Use iPhone and HomePod mini

### Prepare

Upgrade the iPhone OS to 16.1.2 or higher

Confirm that the App Home is installed on your phone

Upgrade HomePod to version 16.1 or above


### Bind iPhone and HomePod

1. Plug the HomePod mini or HomePod into the power supply. Wait for the prompt to sound and the indicator on the HomePod mini or the top of the HomePod starts flashing white.
2. Unlock your iPhone or iPad and place it close to the HomePod mini or HomePod. When "Settings" appears on the device screen, tap this button.
3. When the system prompts you to place the HomePod mini or HomePod in the center of the iPhone or iPad viewfinder, follow the prompts to complete pairing. If you can't use the camera, please click "Enter password manually", and Siri will respond with a four digit password. Please enter this password into your device.
4. Follow the onscreen instructions to select your settings. After setting up, you can also customize and manage all the HomePod mini or HomePod settings in the Home app.
5. Wait for HomePod mini or HomePod to finish setting, then tap Finish.


### Add a Night Light in Home

1. Ensure that Night-Light is connected to the computer

2. UART baud rate is set to 2000000, and power on again

3. From the uart log, copy URL similar to the following:

    `https://project-chip.github.io/connectedhomeip/qrcode.html?data=MT%3A6FCJ142C00KA0648G00`

    <u>*Note: You can print the URL multiple times by short pressing the key*</u>

4. Open the URL above with a browser, and normally, there will be a QR code on the screen

5. Open the mobile phone Home App, click the "+" in the upper right corner, and select "Add accessories" from the pop-up menu

6. Scan the QR code of the night light on the pop-up interface, and add it according to the prompts. You can customize the location and name of the accessories at Home

7. Wait for the configuration to be completed, and there will be a new night light in Home

8. You can open the accessories interface to control the night light through touch, or control the light through voice interaction, such as "turn on all lights"


### Reset Apple HomePod

Power on the loudspeaker after power off for 10s

Wait for 5s after power on, press the middle position of the speaker with your index finger

Release after you hear 3 beeps, according to the speaker prompts, and there will be a chime


### Remove Accessories from Home

Open the Home App and enter "My Home"

Tap the accessory to be deleted to open the accessory details page

Slide the screen to the bottom, select "Remove accessories", and confirm

For HomePod, select "Restore HomePod..." and then "Remove Accessories"



## Use Android Phone and Google Nest

### Prepare

A  Google Nest (speaker or display)

Latest version of the Google Home app

A Google Account

A mobile phone or tablet that:

*Has Android 8.0 or later*

*Works with 2.4 GHz and 5 GHz Wi-Fi network (a WPA-2 Enterprise network won't work)*

*Has Bluetooth turned on*

*An Internet connection and secure wireless network*


### Set up Google Nest

Open the Google Home app (upgrade to the latest version first) on your mobile phone

Recommended to set Google Nest as the first device

Tap "Devices" on the "Settings" screen

Tap the + Add icon, Set up a device (set up a new device or add an existing device or service to your home)

Choose a home (you will be able to control the devices and services in this home) , then tap Next, Enter Looking for devices...

or Choose  Add another home, then tap Next,  enter a Home nickname and address

Wait a moment till you see " Nest Hub found" result text, then Scan the QR code on the Google Nest screen

Tab Connect when "Connect to device"  box pops up

Complete the setting of Google Nest


### Add a Night Light in Home

Similar to adding Google Nest above, you can add a night light in Google Home as follows:

Tap "Devices" on the "Settings" screen

Tap the + Add icon, Set up a device (set up a new device or add an existing device or service to your home)

Choose "New device" item, Choose a home on next screen, then tap Next, Enter Looking for devices...

on next screen, What are you setting up?

select and tap Matter-enabled device, then enter next screen, Scan Matter QR code

Connect this device to your Google Account, tap "I agree" On the bottom right screen, then Connecting device to Google Home...

Wait a moment when you see "Device connected", then tap Done, in your home you can find a new device, you can change its name in the settings (tap Gear icon in the upper right corner)


### Reset Google Nest

If the device has been used before, you can factory reset it as follows:

On the back of the Nest, press Volume Up and Volume Down simultaneously for 10 seconds.

*(when pressing and holding, a message should appear on the screen telling you about the reset)*



## Use Android Phone and Echo V4

Matter-enabled Amazon Echo devices have built-in software to connect and control Matter smart home devices seamlessly. After a customer sets up an Echo, they can connect their devices by saying, "Alexa, discover my devices", or if you like, by adding the device in the Alexa app as follows: 


1. Install Amazon Alexa app on Android mobile phone

2. Set up your Echo speaker according to instructions

3. Open the Alexa app and enter "Device Settings" screen

4. Tap the + sign in the upper right corner to select "Add Device" from the pop-up screen

5. Enter the next screen SETUP and select "Connect your Matter Device"

   "Control your Matter device with Alexa", Next

   "Does your device have a Matter logo ?", Yes

6. "Locate a QR code shown for your device", tap "Scan Qr Code" at the bottom of the screen

7. "Allow Permission:  Bluetooth, Camera", Next 

8. "Allow Amazon Alexa to take pictures and record video ?"

   select "While using the app", then Scan the QR code for Matter Device

   Enter "Looking for your device" screen, wait a moment

   when you see "Connect device to Wi-Fi" select the hotspot your phone is using

9. "Device found and connected", tap "Done"

10. Enter the LIGHTS screen, you can control the lights added above, or voice control lights

    (Currently, it is recommended to use alexa control BL602)


## Factory Reset Night Light

Long press the key for about 4s+, during the process,the light first turns on red and then off
Then release the key, until the yellow light comes on, indicating that factory reset OK



# Questions and Answers

If you encounter any inconvenience in using our night light products, please refer to <a href="https://github.com/thirdreality/thread_nightlight/blob/master/docs_3r/FAQ/FAQ_3R.md">FAQ</a> for help.
