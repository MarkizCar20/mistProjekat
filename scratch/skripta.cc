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
    Ssid ssid = Ssid("ns-3-ssid");

    //setting WiFi standard, setting up NetDevices
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211n);
    NetDeviceContainer wifiStaDevices;
    NetDeviceContainer wifiApDevice;

    //Installation
    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid), "Active-Probing", BooleanValue(false));
    wifiStaDevices = wifi.install(phy, mac, wifiStaNodes);
    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
    wifiApDevice = wifi.install(phy, mac, wifiApNode);

    //Setting up random movement
    MobilityHelper mobility;

    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                "MinX",
                                DoubleValue(0.0),
                                "MinY",
                                DoubleValue(0.0),
                                "DeltaX",
                                DoubleValue(5.0),
                                "DeltaY",
                                DoubleValue(10.0),
                                "GridWidth",
                                UintegerValue(3),
                                "LayoutType",
                                StringValue("RowFirst"));
    
    mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                            "Bounds",
                            RectangleValue(Rectangle(-50, 50, -50, 50)));
    mobility.install(wifiStaNodes);
    
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.install(wifiApNode);

    InternetStackHelper stack;
    stack.Install(wifiApNode);
    stack.Install(wifiStaNodes);

    Ipv4AddressHelper address;
    address.SetBase("192.168.3.0", "255.255.255.0");
    Ipv4InterfaceContainer staNodesInterface;
    Ipv4InterfaceContainer apNodeInterface;

    staNodesInterface = address.Assign(wifiStaDevices);
    apNodeInterface = address.Assign(wifiApDevice);

    ApplicationContainer serverApp;
    UdpEchoServerHelper echoServer(12);

    serverApp = echoServer.install(wifiStaNodes.Get(0));
    serverApp.Start(Seconds(0.0));
    serverApp.Stop(Seconds(11.0));

    UdpClientHelper echoClient(staNodesInterface.GetAddress(0), 12);
    echoClient.SetAttribute("MaxPackets", UintegerValue(4294967295U));
    echoClient.SetAttribute("Interval", TimeValue(Time("0.00001"))); // packets/s
    echoClient.SetAttribute("PacketSize", UintegerValue(1472));
    ApplicationContainer clientApp = echoClient.install(wifiApNode.Get(0));
    clientApp.Start(Seconds(1.0));
    clientApp.Stop(Seconds(11.0)); 


    Simulator::Run();
    Simulator::Destroy();
    return 0;
}