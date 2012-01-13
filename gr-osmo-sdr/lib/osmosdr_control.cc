#include "osmosdr_control.h"

osmosdr_control::osmosdr_control(const std::string &addr)
{
    /* lookup acm control channel device name for a given alsa device name */

    /*
        if (addr.empty())
            pick first available device or throw an exception();
    */
}

osmosdr_control::~osmosdr_control()
{
}

std::vector< std::string > osmosdr_control::find_devices()
{
    std::vector< std::string > devices;

    return devices;
}

std::string osmosdr_control::audio_dev_name()
{
    return "hw:1";
}

std::string osmosdr_control::control_dev_name()
{
    return "/dev/ttyUSB0";
}

osmosdr_rx_control::osmosdr_rx_control(const std::string &addr) :
    osmosdr_control(addr)
{
}

osmosdr_rx_control::~osmosdr_rx_control()
{
}

osmosdr_tx_control::osmosdr_tx_control(const std::string &addr) :
    osmosdr_control(addr)
{
}

osmosdr_tx_control::~osmosdr_tx_control()
{
}
