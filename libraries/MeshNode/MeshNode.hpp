#include "stdint.h"

class MeshNode{
    private:
        uint32_t NodeID = 0;
    public:
        // Default Constructor
        MeshNode(); 
        
        // Initialize and start the access point mode 
        bool init_ap_mode();

        // Initialize and start the stand alone mode
        bool init_sta_mode();

};