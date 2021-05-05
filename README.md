# electra-esp32-analog

Turning an analogic Electra IA003/IA01/IA02 into a smart Home Assistant [lock device](https://www.home-assistant.io/integrations/lock/) using an ESP32-WROOM-32D module and a few extra components.

> **WARNING**: This project is a work in progress. Use at own risk.

Feeling generous and want to support my work, here is [my PayPal link](https://paypal.me/kind3r).

## Intro

After some research and experimentation it was **not possible to power the ESP32 module from the intercom power line** as the power is not always available and the ESP32 WiFi and boot power drain spikes confuse the Electra Call Distributor. So the next logical step was to use a battery to power the device. 

Regular **rechargeable Li-Ion** batteries are commonly available ini different shapes and sizes. The problem is that they charge up to about 4.2V so they require some voltage drop to be able to power the ESP32. Since the device only wakes up when the intercom is riniging and voltage drop circuit consumes power all the time, this makes using such batteries inefficient (and I am a big fan of optimisation). There are of course more efficient LDO's like TI's [TPS62841DGRR](https://ro.mouser.com/ProductDetail/595-TPS62841DGRR) but those are hard to find currently.

At first I opted for a rechargeable **LiFePO4 18650** battery to optimise power usage since it's nominal voltage is 3.2V and it's max charge is 3.6V, thus it does not require power regulation which saves quite some energy. Combined with the fact that the device is mostly in deep sleep mode, this should give a **very long battery life** (few years - still need to make exact measuremets). Unfortunatelly LiFePO4 batteries and chargers (I had to build my own) do not seem to be that common making which raises the cost of (also time to build) the device. 

Now I am testing 2 **alkaline AAA** batteries and it seems to work fine and they also fit in the intercom case, event better than the 18650 but due to their discharge curve only about 30% of their capacity will be used (which still would provide up to a year of usage). But then you are throwinig away 2/3 of the battery (unless you move it to a remote control or some other AAA powered device). Not ideal. 

Another option would be using **lithium AAA** batteries which are available locally (not as much as alkaline AAA, but still). While a bit more expensive than the regular alkaline AAA, their discharge curve is optimal for powering the ESP32, providinig a similar life to the rechargeable LiFePO4. 

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

> **WARNING**: Schematic and PCB not yet final, I might still make adjustments. Rev.4 is currently under testing and working fine. 

> TODO: how to order PCBs and assembly

> TODO: explain extra components required:
> - JST-SH-6 + VoltLink or USB2TTL
> - JST-SH-3 for intercom interface
> - JST-XH-2 and LiFePO4 battery + holder

### Upload software

> TODO: 
> - Connector for uploading
> - How to build and make configuration adjustments (battery reporting interval, configuration mode sleep time)
> - PlatformIO + upload SPIFFS (for web config) & config file (optional)

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
- Test power usage

Medium/long term:
- Make a changelog
- Provide other integrations besides Home Assistant