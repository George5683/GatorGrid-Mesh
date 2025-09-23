# Kicad Design V2.0

## Explanation for the Pinouts on the Pico W:

- GPIO 1-22 and 26-28 are as the schematic states
- VBUS is 5V when connected to micro usb to power the Pico
- VSYS can power the Pico with ~1.8-5.5V input

Schematics Update:
- Added a JST connector for an extenal battery to be connected
- Added a switch after the JSt conncector for powering the Picos
- Added a polyfuse (polyfuse's reset instead of replacing the fuse if its broken)
- Added a Schottky diode (to prevent current backflow and if user connects the battery backwards)
- Added a charging/safety cicuit [TP4056] (for charging the battery and battery management)


