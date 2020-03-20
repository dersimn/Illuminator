[![Photo](https://github.com/dersimn/Illuminator/blob/master/docs/IMG_7829.TRIM.gif?raw=true)](https://raw.githubusercontent.com/dersimn/Illuminator/master/docs/IMG_7829.TRIM.m4v)

Use a WS2812 LED Strip to build an RGB light with support for animations.

## Default settings

Currently the MQTT server IP and Wifi credentials are hard-coded in `default_config.h`.

### Default topics

The default topics are

    Illuminator/status/1234567/fastled
    Illuminator/set   /1234567/fastled

containing/receiving JSON payloads like:

    {
        "val": 0,
        "hue": 0.043137,
        "sat": 1,
        "animation": "sinelon"
    }

Value (Brightness), Hue, Saturation is `0..1`
Animation can be `"none", "sinelon", "confetti", "colorwheel"`.

### Maintenance topics

    Illuminator/maintenance/1234567/online      -> true
    Illuminator/maintenance/1234567/uptime      -> {"val":83000243,"millis":83000243,"rollover":0}
    Illuminator/maintenance/1234567/temperature -> 26.00
    Illuminator/maintenance/1234567/info        -> {"board_id":"Illuminator_1234567","build_hash":"7c7eb13b668fae2f371a07928d9e0facd366c9bf","build_tag":"master","build_timestamp":1584719403,"ip_address":"10.1.1.108"}


## Install

Install using [PlatformIO CLI](https://docs.platformio.org/en/latest/installation.html).  
Clone this repository, `cd` into it.  
If you're not using a Wemos D1 Mini, edit `platformio.ini` first.  

Run:

    platformio run -t upload

For OTA upload:

    platformio run -t upload --upload-port Illuminator_1234567.local

## Development

Ideas:

- Replace FastLED with [WS2812FX](https://github.com/kitesurfer1404/WS2812FX) and make use of [segments](https://github.com/kitesurfer1404/WS2812FX/blob/master/examples/ws2812fx_segments/ws2812fx_segments.ino)


## Credits

This project follows [Oliver "owagner" Wagner](https://github.com/owagner)'s architectural proposal for an [mqtt-smarthome](https://github.com/mqtt-smarthome/mqtt-smarthome).