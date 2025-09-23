# Kicad Design V2.0

## Explanation for the Pinouts on the Pico W:

- GPIO 1-22 and 26-28 are as the schematic states
- VBUS is 5V when connected to micro usb to power the Pico
- VSYS can power the Pico with ~1.8-5.5V input

## Schematic Update:
- Fixed wiring of temp on the TP4056 as it was disconnected which casued the battery to not charge. 
- Changed POWER LED to be driven by two transistors which allows it to turn on when both picos are on. 
- Removed the schottky diode connected to the battery and instead put a diode on each VSYS out from each pico in order to prevent backflow when eitehr are plugged in as VSYS can be input and output. 
- Reworked Wiring on PCB


