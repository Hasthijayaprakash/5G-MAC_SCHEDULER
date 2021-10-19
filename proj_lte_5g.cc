//Author : Hasthi JayaPrakash
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lte-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/config-store-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/animation-interface.h"
#include <ns3/lte-control-messages.h>
#include "ns3/node-list.h"
#include <iostream>
using namespace std;

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("LenaX2HandoverExample");

void
NotifyConnectionEstablishedUe (std::string context,
                               uint64_t imsi,
                               uint16_t cellid,
                               uint16_t rnti)
{
  std::cout << Simulator::Now ().GetSeconds () << " " << context
            << " UE IMSI " << imsi
            << ": connected to CellId " << cellid
            << " with RNTI " << rnti
            << std::endl;
}

void
NotifyHandoverStartUe (std::string context,
                       uint64_t imsi,
                       uint16_t cellid,
                       uint16_t rnti,
                       uint16_t targetCellId)
{
  std::cout << Simulator::Now ().GetSeconds () << " " << context
            << " UE IMSI " << imsi
            << ": previously connected to CellId " << cellid
            << " with RNTI " << rnti
            << ", doing handover to CellId " << targetCellId
            << std::endl;
}

void
NotifyHandoverEndOkUe (std::string context,
                       uint64_t imsi,
                       uint16_t cellid,
                       uint16_t rnti)
{
  std::cout << Simulator::Now ().GetSeconds () << " " << context
            << " UE IMSI " << imsi
            << ": successful handover to CellId " << cellid
            << " with RNTI " << rnti
            << std::endl;
}

void
NotifyConnectionEstablishedEnb (std::string context,
                                uint64_t imsi,
                                uint16_t cellid,
                                uint16_t rnti)
{
  std::cout << Simulator::Now ().GetSeconds () << " " << context
            << " eNB CellId " << cellid
            << ": successful connection of UE with IMSI " << imsi
            << " RNTI " << rnti
            << std::endl;
}

void
NotifyHandoverStartEnb (std::string context,
                        uint64_t imsi,
                        uint16_t cellid,
                        uint16_t rnti,
                        uint16_t targetCellId)
{
  std::cout << Simulator::Now ().GetSeconds () << " " << context
            << " eNB CellId " << cellid
            << ": start handover of UE with IMSI " << imsi
            << " RNTI " << rnti
            << " to CellId " << targetCellId
            << std::endl;
}

void
NotifyHandoverEndOkEnb (std::string context,
                        uint64_t imsi,
                        uint16_t cellid,
                        uint16_t rnti)
{
  std::cout << Simulator::Now ().GetSeconds () << " " << context
            << " eNB CellId " << cellid
            << ": completed handover of UE with IMSI " << imsi
            << " RNTI " << rnti
            << std::endl;
}



int
main (int argc, char *argv[])
{

  uint16_t numberOfUes = 5;
  uint16_t numberOfEnbs = 5;
  uint16_t numBearersPerUe = 2;
  Time simTime = MilliSeconds (490);
        simTime=Seconds (10);

  bool disableDl = false;
  bool disableUl = false;


      int nodeSpeed = 1; //in m/s
      int nodePause = 0; //in s

  // change some default attributes so that they are reasonable for
  // this scenario, but do this before processing command line
  // arguments, so that the user is allowed to override these settings
  Config::SetDefault ("ns3::UdpClient::Interval", TimeValue (Seconds (0.5)));
  Config::SetDefault ("ns3::UdpClient::MaxPackets", UintegerValue (1000000));
  Config::SetDefault ("ns3::LteHelper::UseIdealRrc", BooleanValue (true));

  // Command line arguments
  CommandLine cmd;
  cmd.AddValue ("numberOfUes", "Number of UEs", numberOfUes);
  cmd.AddValue ("numberOfEnbs", "Number of eNodeBs", numberOfEnbs);
  cmd.AddValue ("simTime", "Total duration of the simulation", simTime);
  cmd.AddValue ("disableDl", "Disable downlink data flows", disableDl);
  cmd.AddValue ("disableUl", "Disable uplink data flows", disableUl);
  cmd.Parse (argc, argv);


  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
  Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper> ();
  lteHelper->SetEpcHelper (epcHelper);
  lteHelper->SetSchedulerType ("ns3::RrFfMacScheduler");

 //lteHelper->SetHandoverAlgorithmType ("ns3::NoOpHandoverAlgorithm"); // disable automatic handover
// lteHelper->SetHandoverAlgorithmType ("ns3::A2A4RsrqHandoverAlgorithm");
lteHelper->SetHandoverAlgorithmType ("ns3::A3RsrpHandoverAlgorithm");


  Ptr<Node> pgw = epcHelper->GetPgwNode ();

  // Create a single RemoteHost
  NodeContainer remoteHostContainer;
  remoteHostContainer.Create (1);
  Ptr<Node> remoteHost = remoteHostContainer.Get (0);
  InternetStackHelper internet;
  internet.Install (remoteHostContainer);

  // Create the Internet
  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.010)));
  NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
  //Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1);


  // Routing of the Internet Host (towards the LTE network)
  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
  // interface 0 is localhost, 1 is the p2p device
  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

  NodeContainer ueNodes;
  NodeContainer enbNodes;
  enbNodes.Create (numberOfEnbs);
  ueNodes.Create (numberOfUes);

  // Install Mobility Model
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  ObjectFactory pos;
  pos.SetTypeId ("ns3::RandomRectanglePositionAllocator");
  pos.Set ("X", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=100.0]"));
  pos.Set ("Y", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=100.0]"));

  Ptr<PositionAllocator> taPositionAlloc = pos.Create ()->GetObject<PositionAllocator> ();

    //Enb Position

        positionAlloc->Add(Vector (15, 31, 0));
        positionAlloc->Add(Vector (30, 31, 0));
        positionAlloc->Add(Vector (45, 31, 0));
        positionAlloc->Add(Vector (60, 31, 0));
        positionAlloc->Add(Vector (5, 31, 0));

     //Ue Position

        positionAlloc->Add(Vector (12, 11, 0));
        positionAlloc->Add(Vector (34, 11, 0));
        positionAlloc->Add(Vector (41, 11, 0));
        positionAlloc->Add(Vector (21, 11, 0));
        positionAlloc->Add(Vector (48, 11, 0));




  MobilityHelper mobility;
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

  mobility.SetPositionAllocator (positionAlloc);
  mobility.Install (enbNodes);

  std::stringstream ssSpeed;
  ssSpeed << "ns3::UniformRandomVariable[Min=0.0|Max=" << nodeSpeed << "]";
  std::stringstream ssPause;
  ssPause << "ns3::ConstantRandomVariable[Constant=" << nodePause << "]";
  mobility.SetMobilityModel ("ns3::RandomWaypointMobilityModel",
                                  "Speed", StringValue (ssSpeed.str ()),
                                  "Pause", StringValue (ssPause.str ()),
                                  "PositionAllocator", PointerValue (taPositionAlloc));



  mobility.Install (ueNodes);

  // Install LTE Devices in eNB and UEs
  NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice (enbNodes);
  NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice (ueNodes);

  // Install the IP stack on the UEs
  internet.Install (ueNodes);
  Ipv4InterfaceContainer ueIpIfaces;
  ueIpIfaces = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueLteDevs));

  Ipv4InterfaceContainer enbIpIfaces;
  enbIpIfaces = epcHelper->AssignUeIpv4Address (NetDeviceContainer (enbLteDevs));

  // Attach all UEs to the first eNodeB




lteHelper->AttachToClosestEnb (ueLteDevs, enbLteDevs);




  NS_LOG_LOGIC ("setting up applications");

  // Install and start applications on UEs and remote host
  uint16_t dlPort = 10000;
  //uint16_t ulPort = 20000;

  // randomize a bit start times to avoid simulation artifacts
  // (e.g., buffer overflows due to packet transmissions happening
  // exactly at the same time)
  Ptr<UniformRandomVariable> startTimeSeconds = CreateObject<UniformRandomVariable> ();
  startTimeSeconds->SetAttribute ("Min", DoubleValue (0.05));
  startTimeSeconds->SetAttribute ("Max", DoubleValue (0.06));

 for (uint32_t u = 0; u < 2; ++u)
    {
      Ptr<Node> ue = ueNodes.Get (u);
      // Set the default gateway for the UE
      Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ue->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);

      for (uint32_t b = 0; b < numBearersPerUe; ++b)
        {
          ApplicationContainer clientApps;
          ApplicationContainer serverApps;
          Ptr<EpcTft> tft = Create<EpcTft> ();

          if (!disableDl)
            {
              ++dlPort;

              NS_LOG_LOGIC ("installing UDP DL app for UE " << u);
              UdpClientHelper dlClientHelper (ueIpIfaces.GetAddress (u), dlPort);
              clientApps.Add (dlClientHelper.Install (remoteHost));
              PacketSinkHelper dlPacketSinkHelper ("ns3::UdpSocketFactory",
                                                   InetSocketAddress (Ipv4Address::GetAny (), dlPort));
              serverApps.Add (dlPacketSinkHelper.Install (ue));

              EpcTft::PacketFilter dlpf;
              dlpf.localPortStart = dlPort;
              dlpf.localPortEnd = dlPort;
              tft->Add (dlpf);
            }
          EpsBearer bearer (EpsBearer::NGBR_VIDEO_TCP_DEFAULT);
          lteHelper->ActivateDedicatedEpsBearer (ueLteDevs.Get (u), bearer, tft);

         // Time startTime = Seconds (startTimeSeconds->GetValue ());
          Time startTime =Seconds(5);
          serverApps.Start (startTime);
          clientApps.Start (startTime);
          serverApps.Stop (simTime);

        } // end for b
    }


  // Add X2 interface
 lteHelper->AddX2Interface (enbNodes);


  lteHelper->EnablePhyTraces ();
  lteHelper->EnableMacTraces ();
  lteHelper->EnableRlcTraces ();
  lteHelper->EnablePdcpTraces ();
  Ptr<RadioBearerStatsCalculator> rlcStats = lteHelper->GetRlcStats ();
  rlcStats->SetAttribute ("EpochDuration", TimeValue (Seconds (0.05)));
  Ptr<RadioBearerStatsCalculator> pdcpStats = lteHelper->GetPdcpStats ();
  pdcpStats->SetAttribute ("EpochDuration", TimeValue (Seconds (0.05)));


  // connect custom trace sinks for RRC connection establishment and handover notification
  Config::Connect ("/NodeList/*/DeviceList/*/LteEnbRrc/ConnectionEstablished",
                   MakeCallback (&NotifyConnectionEstablishedEnb));
  Config::Connect ("/NodeList/*/DeviceList/*/LteUeRrc/ConnectionEstablished",
                   MakeCallback (&NotifyConnectionEstablishedUe));
  Config::Connect ("/NodeList/*/DeviceList/*/LteEnbRrc/HandoverStart",
                   MakeCallback (&NotifyHandoverStartEnb));
  Config::Connect ("/NodeList/*/DeviceList/*/LteUeRrc/HandoverStart",
                   MakeCallback (&NotifyHandoverStartUe));
  Config::Connect ("/NodeList/*/DeviceList/*/LteEnbRrc/HandoverEndOk",
                   MakeCallback (&NotifyHandoverEndOkEnb));
  Config::Connect ("/NodeList/*/DeviceList/*/LteUeRrc/HandoverEndOk",
                   MakeCallback (&NotifyHandoverEndOkUe));


     AnimationInterface anim ("animation.xml");
    // anim.UpdateNodeDescription(pgw,"pgw");
    char lb[20];
  for (uint32_t u = 0; u < numberOfUes; ++u)
    {
        Ptr<Node> ue = ueNodes.Get (u);
        sprintf(lb,"Ue%d", ue->GetId());
        anim.UpdateNodeDescription(ue,lb);
        anim.UpdateNodeColor(ue,0,255,0);
   }

  for (uint16_t i = 0; i < numberOfEnbs; i++)
    {
        Ptr<Node> enb = enbNodes.Get (i);
        sprintf(lb,"enb%d", enb->GetId());
        anim.UpdateNodeDescription(enb,lb);
        anim.UpdateNodeColor(enb,0,255,255);
   }

        sprintf(lb,"Server %d", remoteHost->GetId());
        anim.UpdateNodeDescription(remoteHost,lb);
        anim.UpdateNodeColor(remoteHost,255,255,0);

        sprintf(lb,"PGW %d", pgw->GetId());
        anim.UpdateNodeDescription(pgw,lb);

        Ptr<Node> sgw = epcHelper->GetSgwNode ();
        sprintf(lb,"SGW %d", sgw->GetId());
        anim.UpdateNodeDescription(sgw,lb);
        anim.UpdateNodeColor(sgw,255,255,255);


         Ptr<Node> mme = NodeList::GetNode (2);
        sprintf(lb,"MME %d", mme->GetId());
        anim.UpdateNodeDescription(mme,lb);
        anim.UpdateNodeColor(mme,255,0,255);


	FlowMonitorHelper flowmon;
	Ptr<FlowMonitor> monitor = flowmon.InstallAll();

        Simulator::Stop (simTime + MilliSeconds (20));
        Simulator::Run ();

        Simulator::Destroy ();




	monitor->SerializeToXmlFile("out.xml",false,false);
	monitor->CheckForLostPackets();
	Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
	std::map<FlowId,FlowMonitor::FlowStats>stats = monitor->GetFlowStats();
        for(std::map<FlowId,FlowMonitor::FlowStats>::const_iterator in = stats.begin();in!=stats.end();in++)
	{
	Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(in->first);
        if (t.destinationPort == dlPort && in->second.rxBytes)
	{
	std::cout<<" Flow "<<in->first<<"("<<t.sourceAddress<<"->"<<t.destinationAddress<<")\n";
	std::cout<<" Tx Bytes: \t\t"<<in->second.txBytes<<"\n";
	std::cout<<" Rx Bytes: \t\t"<<in->second.rxBytes<<"\n";
	std::cout<<" Tx Packets: \t\t"<<in->second.txPackets<<"\n";
	std::cout<<" Rx Packets: \t\t"<<in->second.rxPackets<<"\n";
	std::cout<<" Packet Delivery Ratio: "<<(float)in->second.rxPackets/in->second.txPackets*100<<"\n";
	std::cout<<" Throughput: \t\t"<<in->second.rxBytes*8.0/30.0 <<" bits/sec\n";
	std::cout<<" Average Delay: \t"<<in->second.delaySum/in->second.rxPackets<<"\n";
	std::cout<<" Average Jitter: \t"<<in->second.jitterSum/in->second.rxPackets<<"\n";
	std::cout<<" Dropped Packets: \t"<<(in->second.txPackets-in->second.rxPackets)<<"\n";
	std::cout<<" Dropping Ratio: \t"<<(float)(in->second.txPackets-in->second.rxPackets)/in->second.txPackets*100<<"\n";
	}
	}

  uint32_t txpkts_[1000];
  uint32_t rxpkts_[1000];
  double   pdr_[1000];
  uint32_t tpt_[1000];
  Time     delay_[1000];
  Time     jitter_[1000];
  uint32_t dropped_pkts_[1000];
  double   dropping_ratio_[1000];


  uint32_t txpkts = 0;
  uint32_t rxpkts = 0;
  double   pdr = 0.0;
  uint32_t tpt = 0;
  Time     delay = Time (0.0);
  Time     jitter = Time (0.0);
  uint32_t dropped_pkts = 0;
  double   dropping_ratio = 0.0;

  uint32_t flow=0;

  for(std::map<FlowId,FlowMonitor::FlowStats>::const_iterator in = stats.begin();in!=stats.end();in++)
  {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(in->first);
	if (t.destinationPort == dlPort && in->second.rxPackets)
        {
  	txpkts_[flow] = in->second.txPackets;
	rxpkts_[flow] = in->second.rxPackets;
  	pdr_[flow] = (float)in->second.rxPackets/in->second.txPackets*100;
  	tpt_[flow] = in->second.rxBytes*8.0/30.0;
	delay_[flow] = in->second.delaySum/in->second.rxPackets;
	jitter_[flow] = in->second.jitterSum/in->second.rxPackets;
  	dropped_pkts_[flow] = (in->second.txPackets-in->second.rxPackets);
  	dropping_ratio_[flow] = (float)(in->second.txPackets-in->second.rxPackets)/in->second.txPackets*100;


        txpkts+=txpkts_[flow];
        rxpkts+=rxpkts_[flow];
	pdr+=pdr_[flow];
	tpt+=tpt_[flow];
	delay+=delay_[flow];
	jitter+=jitter_[flow];
	dropped_pkts+=dropped_pkts_[flow];
	dropping_ratio+=dropping_ratio_[flow];

	flow++;
	}
  }

  std::cout<<"\n"<<"----------------Overall Statistics:----------------"<<"\n";

  std::cout<<" Average Tx Packets: \t"<<txpkts/flow<<"\n";
  std::cout<<" Average Rx Packets: \t"<<rxpkts/flow<<"\n";
  std::cout<<" Packet Delivery Ratio: "<<pdr/flow<<"\n";
  std::cout<<" Throughput: \t\t"<<tpt/flow<<" bits/sec\n";
  std::cout<<" Average Delay: \t"<<delay/flow<<"\n";
  std::cout<<" Average Jitter: \t"<<jitter/flow<<"\n";
  std::cout<<" Dropped Packets: \t"<<dropped_pkts/flow<<"\n";
  std::cout<<" Dropping Ratio: \t"<<dropping_ratio/flow<<"\n";

  return 0;
}
