# MeshNode Library 

The MeshNode library is meant to be used on the cyw43 driver. It allows for functionalities such as access point and stand alone modes to work. 

## Features
- access point mode
- stand alone mode (connecting)

## Functions Overview

### MeshNode

### virtual ~MeshNode()
Default constructor for each Node and initializing a Node-ID

### void set_NodeID(uint32_t ID)
Assigns ID to the NodeID

### uint32_t get_NodeID()
Returns the node id

### void seed_rand()
Generates random seed for node

### Access Point Mode

### bool init_ap_mode()
Constructor that initializes the x, y, and z values to 0

### bool start_ap_mode()
Starts AP mode and server

## Authors
George Salomon, Cole Smith, Jonathon Brown, Mateo Slivka, Maxwell Evans
