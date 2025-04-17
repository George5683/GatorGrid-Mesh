# GatorGrid

GatorGrid is a multifunctional software solution designed for the Raspberry Pi Pico W by UF students to create a mesh network with wireless communication. It provides a low-cost, open-source alternative to existing mesh network solutions like ESP-WIFI-MESH and Zigbee by enabling seamless Wi-Fi-based communication between multiple nodes. The design uses two Pico W's per node in the network The project aims to ensure reliable message relay, even in case of node failures, making it ideal for individuals and organizations needing decentralized communication in remote or disaster-affected areas. GatorGrid focuses on accessibility, scalability, and real-time data transmission.

## Features
- MeshNode library for STA and AP mode
- Doxygen for documentation
- SPI interaction for internode communication

## Requirements to run
- Install doxygen
- Install graphviz
- Install extensions for VS code if applicable: Doxygen Runner, Doxygen Documentation Generator, Live Preview

## How to run Doxygen
- Without Vs code, use doxygen Doxyfile then view it on github or on a website
- With Vs code, do CTRL+SHIFT+P and then type Generate Doxygen Documentation and the viewer should automatically open
- If the viewer does not automatically open, check the settings of Doxygen Runner

## How to upload code to PICO W's
1. Push the bootsel and plug the microcontroller into the computer
2. Create the .uf2 file and upload it to PICO W by dragging it into the directory

## Author
George Salomon, Cole Smith, Jonathon Brown, Mateo Slivka, Maxwell Evans

