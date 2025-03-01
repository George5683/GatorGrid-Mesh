class cyw43{
    public:
        bool sta_mode_enable();

        bool ap_mode_enable(const char *ssid, const char *password, uint32_t auth);

        bool init();

        bool bluetooth_enable();
        
}