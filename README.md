# electra-esp32-analog

Turning an analogic Electra IA003/IA01/IA02 into a smart Home Assistant [lock device](https://www.home-assistant.io/integrations/lock/) using an ESP32-WROOM-32D module and a few extra components.

Feeling generous and want to support my work, here is [my PayPal link](https://paypal.me/kind3r).

## Intro

After some research and experimentation it was **not possible to power the ESP32 module from the intercom power line** as the power is not always available and the ESP32 WiFi and boot power spikes confuse the Electra Call Distributor. So the next logical step was to use a battery to power. I opted for a **LiFePO4 18650** battery to optimise power usage since it's nominal voltage is 3.2V and it's max charge is 3.6V, thus it does not require power regulation which saves quite some energy. Combined with the fact that the device is mostly in deep sleep mode, this should give a **very long battery life** (few years - still need to make exact measuremets). 

## Features

- Easy configuration via Soft AP and browser UI
  - Automatically enter config mode in case of errors
  - Manually enter config mode using dedicated button
- [Home Assistant](https://www.home-assistant.io/) integration via MQTT
  - Ringing status
  - Unlock
  - Battery life reporting
  - Configure automations
- Ultra low power usage
  - Deep sleep when not ringing
  - Deep sleep after 5 min if in configuration mode 
- LED status
  - Slow blink: Configuration mode
  - Fast blink: Ringing
- Optically isolated from intercom's lines, no risk of damaging the ESP32 module
- Fits inside the intercom's enclosure

## How To

### Build your device

Schematic, PCB and BOM are available [here](https://oshwlab.com/Gibonii/electra-ia003-ia01-esp). 

**WARNING: Schematic and PCB not yet final, I might still make adjustments.**  

> TODO: how to order PCBs and assembly

> TODO: explain extra components required:
> - JST-SH-6 + VoltLink or USB2TTL
> - JST-SH-3 for intercom interface
> - JST-XH-2 and LiFePO4 battery + holder

### Upload software

> TODO: 
> - Connector for uploading
> - How to build and make configuration adjustments (battery reporting interval, configuration mode sleep time)
> - PlatformIO + upload SPIFFS & config file

### Configuration

> TODO: 
> - HA preparations
> - First time configuration
> - Testing

## Resources of inspiration

- [https://micro-flight.blogspot.com/2010/09/hack-interfon-electra.html](https://micro-flight.blogspot.com/2010/09/hack-interfon-electra.html)
- [https://forum.arduino.cc/index.php?topic=659051.0](https://forum.arduino.cc/index.php?topic=659051.0)

## TODO

Short term:
- Finish documentation
- Add credits and resources
- Test Rev3 PCB
- Test power usage

Long term:
- Look into using 2xAAA batteries for power, if they can fit in the intercom case
- Look into coin cell powering using CR2450 + supercap
- Look into using BLE for communication (downside is this would require a gateway)