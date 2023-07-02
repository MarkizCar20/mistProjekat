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

    }

    ApplicationContainer serverApp;
    UdpServerHelper echoServer(12);

    serverApp = echoServer.Install(wifiStaNodes.Get(0));
    serverApp.Start(Seconds(0.0));
    serverApp.Stop(Seconds(15.0));

    UdpClientHelper echoClient(staNodesInterface.GetAddress(0), 12);
    echoClient.SetAttribute("MaxPackets", UintegerValue(4294967295U));
    echoClient.SetAttribute("Interval", TimeValue(Time("0.00001"))); // packets/s
    echoClient.SetAttribute("PacketSize", UintegerValue(1472));
    ApplicationContainer clientApp = echoClient.Install(wifiApNode.Get(0));
    clientApp.Start(Seconds(1.0));
    clientApp.Stop(Seconds(14.0)); 

    Simulator::Stop(Seconds(15));
    Simulator::Run();

    double throughput = 0;

    uint64_t totalPacketsThrough = DynamicCast<UdpServer>(serverApp.Get(0))->GetReceived();
    throughput = totalPacketsThrough * 1472 * 8 / (12 * 1000000.0);
    
    std::cout << throughput << " Mbit/s" << std::endl;

    Simulator::Destroy();
    return 0;
}