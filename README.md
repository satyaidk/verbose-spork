# Octo A Desk Gadget 1.0

Two apps on one ESP32-C3 SuperMini, sharing one screen, one touch pad and one
web controller.

    APP 1  MOCHI   16 animations, 1232 frames, ~14 FPS
    APP 2  CLOCK   clock / weather / crypto prices / your own notes

**Hold the touch pad for 15 seconds to switch between them**, or use the
Mochi / Clock switch at the top of the web page.

## Install
1. Copy the whole `MochiWeb` folder into your Arduino sketchbook and open `MochiWeb.ino`.
2. Libraries: **Adafruit SSD1306**, **Adafruit GFX**.
3. Board: ESP32C3 Dev Module, **USB CDC On Boot = Enabled**,
   **Partition Scheme = Huge APP (3MB No OTA/1MB SPIFFS)**.

> The partition scheme is required, not a suggestion. Wi-Fi + HTTP server +
> mDNS is about 1.1 MB and the animations add 207 KB, so the build is ~1.35 MB.
> The default partition gives only 1.31 MB and the compile fails with
> "text section exceeds available space in board". Huge APP gives 3 MB.

## First boot
No Wi-Fi is stored, so the Mochi starts its own hotspot:

    SSID      MOCHI-xxxx        (xxxx = last 4 of the MAC)
    Password  12345678
    Open      http://192.168.4.1/

Enter your network on the System tab. It reboots and afterwards lives at
`http://mochi.local/`.

## Wiring
| Signal | GPIO |
|---|---|
| OLED SDA | 20 |
| OLED SCL | 21 |
| TTP223 out | 1 |
| Buzzer | 2 |

Pins are changeable from the Hardware tab. Use 22 to mean "not fitted".

## Controls

| Gesture | Mochi app | Clock app |
|---|---|---|
| Tap | tap reaction clip | next screen |
| Double tap | double reaction clip | next screen |
| Triple tap | triple reaction clip | next screen |
| Hold 5 s | hold reaction clip | refresh weather + prices |
| Hold 15 s | switch to Clock | switch to Mochi |

The 5 s action fires **on release**; the 15 s switch fires **while held**. A
progress bar appears at 5 s and fills toward 15 s so you can see which you are
about to get.

## Files
    MochiWeb.ino       orchestration, gesture handlers
    config.h           defaults and limits
    settings.h         struct S + NVS persistence
    melody.h           "C4 100 20" language, non-blocking buzzer
    display_mochi.h    SSD1306 bring-up and blitting
    player.h           frame decoder + animation state machine
    clock_app.h        the four info screens and their data sources
    touch.h            five gestures from one pin
    app_mode.h         the switch between the apps, and gesture routing
    web_ui.h           the controller page, in flash
    web_api.h          Wi-Fi + REST API
    animations.h       index of clips
    anim_*.h           16 compressed animations
    tools/             gif2mochi.py, the GIF converter
    tests/             host-side test harness (./run.sh)

See `MochiWeb_v2_Documentation.pdf` for the full explanation.

## Credit
Frames converted from GIFs in
github.com/huykhoong/esp32_dasai_mochi_clone_and_how_to, captured from a Dasai
Mochi product video. Personal use only; replace the artwork before shipping
anything.
