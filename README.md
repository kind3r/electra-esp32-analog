# electra-ia003-esp

## TODO

Document the project:  
- Purpose  
- Schematic, BOM, where to manufacture, share EasyEDA project
- Configuration and how to program

Treat MQTT error due do WiFi hickup:

```
I (1297) wifi:AP's beacon interval = 102400 us, DTIM period = 1
E (7657) esp-tls: couldn't get hostname for :aaa.bbb.ccc:
E (7657) esp-tls: Failed to open new connection
E (7657) TRANS_SSL: Failed to open a new connection
E (7657) MQTT_CLIENT: Error transport connect
I (7667) HomeAssistant: MQTT_EVENT_ERROR
I (7667) Sleep: Monitor started
I (7677) HomeAssistant: MQTT_EVENT_DISCONNECTED
```