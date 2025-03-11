# MeshNode Library 

The MeshNode library is meant to be used on the cyw43 driver. It allows for functionalities such as access point and stand alone modes to work. Enables multiple picos to be connected to each other in a mesh node fashion.

## Features
- access point mode
- stand alone mode (connecting)

## Functions Overview

### **MeshNode**

### `void MeshNode()`
Default constructor for each Node and initializing a Node-ID using the cyw43 driver 

### `virtual ~MeshNode()`
Deconstructor for MeshNode

### `void set_NodeID(int ID)`
Setting the NodeID variable to be an integer ID passed in

### `int get_NodeID()`
Returns the NodeID of the Node object

### **Access Point Mode**

### `bool init_ap_mode()`

## Author
George Salomon, Cole Smith, Jonathon Brown, Mateo Slivka, Maxwell Evans

