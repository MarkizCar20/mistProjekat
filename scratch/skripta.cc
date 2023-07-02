//Simulating a multi node WiFi transfer network using the 802.11 family of Wifi Protocols
//Simulation consists of both TCP and UDP transfer of data, with a extra non-Wi-fi interfer as optional.
// Network topology:
//
//                  AP 
//                  |
// STA -- n1        no         n2 -- InterNode
//        


#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/boolean.h"
#include "ns3/config.h"
#include "ns3/double.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/mobility-helper.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/ssid.h"
#include "ns3/string.h"
#include "ns3/udp-client-server-helper.h"
#include "ns3/uinteger.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/yans-wifi-helper.h" 
#include "ns3/wifi-net-device.h"
#include "ns3/netanim-module.h"
#include "ns3/waveform-generator-helper.h"
#include "ns3/waveform-generator.h"
#include "ns3/string.h"
#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/mobility-helper.h"
#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/non-communicating-net-device.h"
#include "ns3/on-off-helper.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/packet-sink.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/spectrum-wifi-helper.h"

using namespace ns3;

//Global variables for use in callbacks.
double g_signalDbmAvg;  //!< Average signal power [dBm]
double g_noiseDbmAvg;   //!< Average noise power [dBm]
uint32_t g_samples;     //!< Number of samples

/**
 * Monitor sniffer Rx trace
 *
 * \param packet The sensed packet.
 * \param channelFreqMhz The channel frequency [MHz].
 * \param txVector The Tx vector.
 * \param aMpdu The aMPDU.
 * \param signalNoise The signal and noise dBm.
 * \param staId The STA ID.
 */

void
MonitorSniffRx(Ptr<const Packet> packet,
                uint16_t channelFreqMhz,
                WifiTxVector txVector,
                MpduInfo aMpdu,
                SignalNoiseDbm signalNoise,
                uint16_t staId)
{
    g_samples++;
    g_signalDbmAvg += ((signalNoise.signal - g_signalDbmAvg) / g_samples);
    g_noiseDbmAvg += ((signalNoise.noise - g_noiseDbmAvg) / g_samples);
}

NS_LOG_COMPONENT_DEFINE("mistSkripta");

Ptr<SpectrumModel> SpectrumModelWifi5180MHz; //Spectrum model at 5180 Mhz
Ptr<SpectrumModel> SpectrumModelWifi5190MHz; //Spectrum model at 5190 MHz

/*Initializer for spectrum model at 5180Mhz*/
class static_SpectrumModelWifi5180MHz_initializer
{
    public:
        static_SpectrumModelWifi5180MHz_initializer()
        {
            BandInfo bandInfo;
            bandInfo.fc = 5180e6;
            bandInfo.fl = 5180e6 - 10e6;
            bandInfo.fl = 5180e6 + 10e6;

            Bands bands;
            bands.push_back(bandInfo);

            SpectrumModelWifi5180MHz = Create<SpectrumModel>(bands);
        }
}

//Initializing static instance of Spectrum Model at 5180Mhz
static_SpectrumModelWifi5180MHz_initializer static_SpectrumModelWifi5180MHz_initializer_instance;

/*Initializer for spectrum model at 5190Mhz*/
class static_SpectrumModelWifi5190MHz_initializer
{
    public:
        static_SpectrumModelWifi5190MHz_initializer()
        {
            BandInfo bandInfo;
            bandInfo.fc = 5190e6;
            bandInfo.fl = 5190e6 - 10e6;
            bandInfo.fl = 5190e6 + 10e6;

            Bands bands;
            bands.push_back(bandInfo);

            SpectrumModelWifi5190MHz = Create<SpectrumModel>(bands);
        }
}

//Initializing static instance of Spectrum Model at 5190Mhz
static_SpectrumModelWifi5190MHz_initializer static_SpectrumModelWifi5190MHz_initializer_instance;


int 
main(int argc, char* argv[])
{
    bool udp = true;
    double distance = 15;
    double simulationTime = 20; //in seconds
    //uint16_t index = 256;
    string wifiType = "ns3::SpectrumWifiPhy";
    string errorModelType = "ns3::NistErrorRateModel";
    bool enablePcap = false;
    const uint32_t tcpPacketSize = 1448;
    double waveformPower = 0;

    CommandLine cmd(__FILE__);
    cmd.AddValue("simulationTime", "Simulation time in seconds", simulationTime);
    cmd.AddValue("udp", "UDP: If set to 1, otherwise TCP", udp);
    cmd.AddValue("distance", "Distance between nodes in meters", distance);
    cmd.AddValue("index", "restrict index between 0 and 31", index);
    cmd.AddValue("wifiType", "select ns3::SpectrumWifiPhy or ns3::YansWifiPhy", wifiType);
    cmd.AddValue("errorModelType", "select ns3::NistErrorRateModel or ns3::YansErrorRateModel",
                 errorModelType);
    cmd.AddValue("enablePcap", "Enabling Pcap tracing output", enablePcap);
    cmd.AddValue("waveformPower", "Waveform power (linear W)", waveformPower);
    cmd.Parse(argc, argv);

    std::cout << "wifiType: " << wifiType << ", distance: " << distance
              << "m, time: " << simulationTime << ", TxPower: 16 dmb(40mW)"<< std::endl; 
    //Add output of final program

    for (uint16_t i = startIndex; i <= stopIndex; i ++ )
    { 
        uint32_t payloadSize;
        if (udp) 
        {
            payloadSize = 972; //1000 bytes of IPv4
        }
        else
        {
            payloadSize = 1448; //1500 bytes of IPv6
            Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(payloadSize));
        }

        NodeContainer wifiStaNodes;
        wifiStaNodes.Create(1);
        NodeContainer wifiApNode;
        wifiApNode.Create(1);
        NodeContainer interferingNode;
        interferingNode.Create(1);

        YansWifiPhyHelper phy;
        SpectrumWifiPhyHelper spectrumPhy;
        Ptr<MultiModelSpectrumChannel> spectrumChannel;
        uint16_t frequency = (i <= 15 ? 5180:5190);
        if (wifiType = "ns3::YansWifiPhy")
        {
            YansWifiChannelHelper channel;
            channel.AddPropagationLoss("ns3::FriisPropagationLossModel",
                                       "Frequency", DoubleValue(frequency * 1e6));
            channel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
            phy.SetChannel(channel.Create());
            phy.Set("ChannelSettings",
                    StringValue(string("{") + (frequency == 5180 ? "36" : 38) +
                                ", 0, BAND_5GHZ, 0}"));
        }
        else if (wifiType == "ns3::SpectrumWifiPhy")
        {
            spectrumChannel = CreateObject<MultiModelSpectrumChannel>();
            Ptr<FriisPropagationLossModel> lossModel = CreateObject<FriisPropagationLossModel>();
            lossModel -> SetFrequency(frequency * 1e6);
            spectrumChannel -> AddPropagationLossModel(lossModel);

            Ptr<ConstantSpeedPropagationDelayModel> delayModel = CreateObject<ConstantSpeedPropagationDelayModel>();
            spectrumChannel -> SetPropagationDelayModel(delayModel);

            spectrumPhy.SetChannel(spectrumChannel);
            spectrumPhy.SetErrorRateModel(errorModelType);
        
            spectrumPhy.Set("ChannelSettings",
                            StringValue(std::string("{") + (frequency == 5180 ? "36" : "38") +
                                        ", 0, BAND_5GHZ, 0}"));
        
        }
        else 
        {
            NS_FATAL_ERROR("Unsupported WiFi type: " << wifiType);
        }

        WifiHelper wifi;
        wifi.SetStandard(WIFI_STANDARD_80211n);
        WifiMacHelper mac;

        Ssid ssid = Ssid("ns380211n");

        double datarate = 0;
        StringValue DataRate;
        double value1 = 6.5;
        double value2 = 7.2;
        double value3 = 13.5;
        double value4 = 15;
        switch(i) 
        {
            case 0: 
            { 
                DataRate = StringValue("HtMcs0");
                datarate = value1*(i+1);
                break;
            }
            case 1:
            { 
                DataRate = StringValue("HtMcs1");
                datarate = value1*(i+1);
                break;
            }
            case 2:
            { 
                DataRate = StringValue("HtMcs2");
                datarate = value1*(i+1);
                break;
            }
            case 3:
            { 
                DataRate = StringValue("HtMcs3");
                datarate = value1*(i+1);
                break;
            }
            case 4:
            { 
                DataRate = StringValue("HtMcs4");
                datarate = value1*(i+1);
                break;
            }
            case 5:
            { 
                DataRate = StringValue("HtMcs5");
                datarate = value1*(i+1);
                break;
            }
            case 6:
            { 
                DataRate = StringValue("HtMcs6");
                datarate = value1*(i+1);
                break;
            }
            case 7:
            { 
                DataRate = StringValue("HtMcs7");
                datarate = value1*(i+1);
                break;
            }
            case 8:
            { 
                DataRate = StringValue("HtMcs0");
                datarate = value2;
                break;
            }
            case 9:
            { 
                DataRate = StringValue("HtMcs1");
                datarate = value2*2;
                break;
            }
            case 10:
            { 
                DataRate = StringValue("HtMcs2");
                datarate = value2*3;
                break;
            }
            case 11:
            { 
                DataRate = StringValue("HtMcs3");
                datarate = value2*3;
                break;
            }
            case 12:
            { 
                DataRate = StringValue("HtMcs4");
                datarate = value2*4;
                break;
            }
            case 13:
            { 
                DataRate = StringValue("HtMcs5");
                datarate = value2*5;
                break;
            }
            case 14:
            { 
                DataRate = StringValue("HtMcs6");
                datarate = value2*6;
                break;
            }
            case 15:
            { 
                DataRate = StringValue("HtMcs7");
                datarate = value2*7;
                break;
            }
            case 16:
            { 
                DataRate = StringValue("HtMcs0");
                datarate = value3;
                break;
            }
            case 17:
            { 
                DataRate = StringValue("HtMcs1");
                datarate = value3*2;
                break;
            }
            case 18:
            { 
                DataRate = StringValue("HtMcs2");
                datarate = value3*3;
                break;
            }
            case 19:
            { 
                DataRate = StringValue("HtMcs3");
                datarate = value3*4;
                break;
            }
            case 20:
            { 
                DataRate = StringValue("HtMcs4");
                datarate = value3*5;
                break;
            }
            case 21:
            { 
                DataRate = StringValue("HtMcs5");
                datarate = value3*6;
                break;
            }
            case 22:
            { 
                DataRate = StringValue("HtMcs6");
                datarate = value3*7;
                break;
            }
            case 23:
            { 
                DataRate = StringValue("HtMcs7");
                datarate = value3*8;
                break;
            }
            case 24:
            { 
                DataRate = StringValue("HtMcs0");
                datarate = value4;
                break;
            }
            case 25:
            { 
                DataRate = StringValue("HtMcs1");
                datarate = value4*2;
                break;
            }
            case 26:
            { 
                DataRate = StringValue("HtMcs2");
                datarate = value4*3;
                break;
            }
            case 27:
            { 
                DataRate = StringValue("HtMcs3");
                datarate = value4*4;
                break;
            }
            case 28:
            { 
                DataRate = StringValue("HtMcs4");
                datarate = value4*5;
                break;
            }
            case 29:
            { 
                DataRate = StringValue("HtMcs5");
                datarate = value4*6;
                break;
            }
            case 30:
            { 
                DataRate = StringValue("HtMcs6");
                datarate = value4*7;
                break;
            }
            default: 
            { 
                DataRate = StringValue("HtMcs7");
                datarate = value4*8;
                break;
            }
        }

        wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                     "DataMode", DataMode,
                                     "ControlMode", DataRate);
        
        NetDeviceContainer staDevice;
        NetDeviceContainer apDevice;
        /* Setting up all the macs for both Yans and Spectrum, installing devices */

        if (wifiType = "ns3::YansWifiPhy")
        {
            mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid));
            staDevice = wifi.Install(phy, mac, wifiStaNode);
            mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
            apDevice = wifi.install(phy, mac, wifiApNode);
        }
        else
        {
            mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid));
            staDevice = wifi.Install(spectrumPhy, mac, wifiStaNode);
            mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
            apDevice = wifi.Install(spectrumPhy, mac, wifiApNode);
        }

        if (i <= 7)
        {
            Config::Set("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/HtConfiguration/"
                        "ShortGuardIntervalSupported",
                        BooleanValue(false));
        }
        else if (i > 7 && i <= 15)
        {
            Config::Set("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/HtConfiguration/"
                        "ShortGuardIntervalSupported",
                        BooleanValue(false));
        }
        else
        {
            Config::Set("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/HtConfiguration/"
                        "ShortGuardIntervalSupported",
                        BooleanValue(true));
        }

        /* Setting up Mobility for all three nodes */
        MobilityHelper mobility;
        Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();

        positionAlloc->Add(Vector(0.0, 0.0, 0.0));
        positionAlloc->Add(Vector(distance, 0.0, 0.0));
        positionAlloc->Add(Vector(distance, distance, 0.0));
        mobility.SetPositionAllocator(positionAlloc);

        mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

        mobility.Install(wifiApNode);
        mobility.Install(wifiStaNode);
        mobility.Install(interferingNode);

        /* Internet stack */
        InternetStackHelper stack;
        stack.Install(wifiApNode);
        stack.Install(wifiStaNode);

        Ipv4AddressHelper address;
        address.SetBase("192.168.1.0", "255.255.255.0");
        Ipv4InterfaceContainer staNodeInterface;
        Ipv4InterfaceContainer apNodeInterface;

        staNodeInterface = address.Assign(staDevice);
        apNodeInterface = address.Assign(apDevice);

        /* Setting up UDP and TCP Apps */
        if (udp)
        {
            //Udp setup
            uint16_t port = 12;
            UdpServerHelper server(port);
            serverApp = server.Install(wifiStaNode.Get(0));
            serverApp.Start(Seconds(0.0));
            serverApp.Stop(Seconds(simulationTime + 1));

            UdpClientHelper client(staNodeInterface.GetAddress(0), port);
            client.SetAttribute("MaxPackets", UintegerValue(4294967295U));
            client.SetAttribute("Interval", TimeValue(Time("0.0001"))); //
            client.SetAttribute("PacketSize", UintegerValue(payloadSize));
            ApplicationContainer clientApp = client.Install(wifiApNode.Get(0));
            clientApp.Start(Seconds(1.0));
            clientApp.Stop(Seconds(simulationTime + 1));
        }
        else
        {
            //TCP setup
            uint16_t port = 50000;
            Address localAddress(InetSocketAddress(Ipv4Address::GetAny(), port));
            PacketSinkHelper packetSinkHelper("ns3::SocketTcpFactory", localAddress);
            serverApp = packetSinkHelper.Install(wifiStaNode.Get(0));
            serverApp.Start(Seconds(0.0));
            serverApp.Stop(Seconds(simulationTime + 1));

            OnOffHelper onoff("ns3::TcpSocketFactory", Ipv4Address::GetAny());
            onoff.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
            onoff.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
            onoff.SetAttribute("PacketSize", UintegerValue(payloadSize));
            onoff.SetAttribute("DataRate", DataRateValue(1000000000)); bit/s

            AddressValue remoteAddress(InetSocketAddress(staNodeInterface.GetAddress(0), port));
            onoff.SetAttribute("Remote", remoteAddress);
            ApplicationContainerContainer clientApp = onoff.Install(wifiApNode.Get(0));
            clientApp.Start(Seconds(1.0));
            clientApp.Stop(Seconds(simulationTime + 1));
        }

        /* Confinguring Waveform generation */
        Ptr<SpectrumValue> wgPsd = Create<SpectrumValue>(i <= 15 ? SpectrumModelWifi5180MHz : SpectrumModelWifi5190MHz);
        *wgPsd = waveformPower / 20e6; /* The power spred over 20Mhz */
        NS_LOG_INFO("wgPsd: " << *wgPsd << " Integrated power: " < Integral(*(GetPointer(wgPsd))));

        if (wifiType = "ns3::SpectrumWifiPhy")
        {
            WaveformGeneratorHelper waveformGeneratorHelper;
            waveformGeneratorHelper.SetChannel(spectrumChannel);
            waveformGeneratorHelper.SetTxPowerSpectralDensity(wgPsd);

            waveformGeneratorHelper.SetPhyAttribute("Period", TimeValue(Seconds(0.0007)));
            waveformGeneratorHelper.SetPhyAttribute("DutyCycle", DoubleValue(1));
            NetDeviceContainer waveformGeneratorHelper.Install(interferingNode);

            Simulator::Schedule(Seconds(0.002), &WaweformGenerator::Start,
                                                WaweformGeneratorDevices.Get(0)
                                                    ->GetObject<NonCommunicatingNetDevice>()
                                                    ->GetPhy()
                                                    ->GetObject<WaweformGenerator>());
        }

        Config::ConnectWithoutContext("/NodeList/0/DeviceList/*/Phy/MonitorSnifferRx", MakeCallback(&MonitorSniffRx));

        if (enablePcap)
        {
            phy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
            std::stringstream ss;
            ss << "wifi-spectrum-po-primeru" << i;
            phy.EnablePcap(ss.str(), apDevice);
        }
        g_signalDbmAvg = 0;
        g_noiseDbmAvg = 0;
        g_samples = 0;

        /* Making sure everything is tuned properly */
        Ptr<NetDevice> staDevicePtr = staDevice.Get(0);
        Ptr<WifiPhy> wifiPhyPtr = staDevicePtr->GetObject<WifiNetDevice>()->GetPhy();
        if (i <= 15)
        {
            NS_ABORT_MSG_IF(wifiPhyPtr->GetChannelWidth() != 20,
                            "Error: Channel width must be 20Mhz for MCS index <=15");
            NS_ABORT_MSG_IF(
                wifiPhyPtr->GetFrequency() != 5180,
                            "Error: Channel frequency must be tuned to 5180 Mhz");
        }
        else
        {
            NS_ABORT_MSG_IF(wifiPhyPtr->GetChannelWidth() != 40,
                            "Error: Channel width must be 40Mhz for MCS Index >15");
            NS_ABORT_MSG_IF(
                wifiPhyPtr->GetFrequency() != 5190,
                            "Error: Channel frequency must be tuned to 5190 Mhz")
        }

        Simulator::Run();
        
        double throughput = 0;
        uint64_t totalPacketsThrough = 0;
        if (udp)
        {
            /* UDP Packet counts */
            totalPacketsThrough = DynamicCast<UdpServer>(serverApp.Get(0))->GetReceived();
            throughput = totalPacketsThrough * payloadSize * 8 / (simulationTime * 1000000.0); /*Mbits/s*/
        }
        else
        {
            /* TCP Packet counts */
            uint64_t totalBytesRx = DynamicCast<PacketSink>(serverApp.Get(0))->GetTotalRx();
            totalPacketsThrough = totalBytesRx / tcpPacketSize;
            throughput = totalBytesRx * 8/ (simulationTime * 1000000.0);
        }
        std::cout << std::setw(5) << i << std::setw(6) << (i % 8) << std::setprecision(2)
                  << std::fixed << std::setw(10) << datarate << std::setw(12) << throughput
                  << std::setw(8) << totalPacketsThrough;
        if (totalPacketsThrough > 0)
        {
            std::cout << std::setw(12) << g_signalDbmAvg << std::setw(12) << g_noiseDbmAvg
                      << std::setw(12) << (g_signalDbmAvg - g_noiseDbmAvg) << std::endl;
        }
        else
        {
            std::cout << std::setw(12) << "N/A" << std::setw(12) << "N/A" << std::setw(12) << "N/A"
                      << std::endl;
        }
        Simulator::Destroy();
    }

   

    return 0;
}