//Simulating a multi node WiFi transfer network using the 802.11 family of Wifi Protocols
//
// Network topology:
//
//                  AP 
//                  |
// STA -- n1        no         n4 -- STA
//        
//              n2          n3
//              |           |
//             STA         STA
//Network characteristics: 

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/ssid.h"
#include "ns3/yans-wifi-helper.h" 

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("mistSkripta");

int 
main(int argc, char* argv[])
{
    bool say = true;

    if (say) {
        LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
        LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);
    }

    NodeContainer wifiStaNodes;
    wifiStaNodes.create(10);
    NodeContainer wifiApNode;
    wifiApNode.create(1);

    //creating a channel
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy = YansWifiPhyHelper::Default();
    phy.SetChannel(channel.create());

    //mac type used for installation
    WifiMacHelper mac;

    //setting WiFi standard, setting up NetDevices
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211n);
    NetDeviceContainer wifiStaDevices;
    NetDeviceContainer wifiApDevice;

    //Installation
    mac.SetType("ns3::StaWifiMac");
    wifiStaDevices = wifi.install(phy, mac, wifiStaNodes);
    mac.SetType("ns3::ApWifiMac");
    wifiApNode = wifi.install(phy, mac, wifiApNode);

}