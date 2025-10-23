# GatorGrid

GatorGrid is a multifunctional software solution designed for the Raspberry Pi Pico W by UF students to create a mesh network with wireless communication. It provides a low-cost, open-source alternative to existing mesh network solutions like ESP-WIFI-MESH and Zigbee by enabling seamless Wi-Fi-based communication between multiple nodes. The design uses two Pico W's per node in the network The project aims to ensure reliable message relay, even in case of node failures, making it ideal for individuals and organizations needing decentralized communication in remote or disaster-affected areas. GatorGrid focuses on accessibility, scalability, and real-time data transmission.

## Features
- MeshNode library for STA and AP mode
- Doxygen for documentation
- SPI interaction for internode communication
- Kicad Custom PCB deisgn with external battery charging capabilities

## Requirements to run
- Install doxygen
- Install graphviz
- Install extensions for VS code if applicable: Doxygen Documentation Generator 

## How to run Doxygen
1. Use doxygen Doxyfile to generate the documentation
2. Use **start doc\html\index.html** (WINDOWS ONLY) or **open doc/html/index.html** (LINUX/MAC)

## How to upload code to PICO W's
1. Push the bootsel and plug the microcontroller into the computer
2. Create the .uf2 file and upload it to PICO W by dragging it into the directory

# PCB
The PCB is deisgned to have two Pico W's connected to each of the two vertical slots.

![PCB Image](/img/pcb.jpeg)

## Bugs:
- At the moment if in an environment with high congestion, packets may not arrive. Moving locations is required.
- SPI has a slight delay of 110 ms per message sent. 

## Author
George Salomon, Cole Smith, Jonathon Brown, Mateo Slivka, Maxwell Evans

