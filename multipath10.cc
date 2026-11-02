#include "ns3/netanim-module.h"
#include <string>
#include <cstdlib>
#include <fstream>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/netanim-module.h"


using namespace ns3;

uint32_t maxQueueSize = 0;
uint64_t pathABytesTransmitted = 0;
uint64_t pathBBytesTransmitted = 0;

double earlyPacketsSent = 0;
double earlyPacketsReceived = 0;
double earlyPacketsLost = 0;
double earlyDelay = 0;
double earlyJitter = 0;
double earlyThroughput = 0;
double earlyQueue = 0;
double earlyUtilization = 0;

void CountPathATransmission(Ptr<const Packet> packet)
{
    pathABytesTransmitted += packet->GetSize();
}

void CountPathBTransmission(Ptr<const Packet> packet)
{
    pathBBytesTransmitted += packet->GetSize();
}

void RecordNetworkTrace(Ptr<Queue<Packet>> queue)
{
    std::ofstream trace("live_network/data/network_trace.csv", std::ios::app);
    trace << Simulator::Now().GetSeconds() << ","
          << queue->GetNPackets() << ","
          << pathABytesTransmitted << ","
          << pathBBytesTransmitted << ","
          << "SAMPLE" << "\n";
    trace.close();

    if (Simulator::Now() + MilliSeconds(100) <= Seconds(15.0))
    {
        Simulator::Schedule(
            MilliSeconds(100),
            &RecordNetworkTrace,
            queue);
    }
}

void MonitorQueue(Ptr<Queue<Packet>> queue)
{
    uint32_t currentSize = queue->GetNPackets();

    if (currentSize > maxQueueSize)
    {
        maxQueueSize = currentSize;
    }

    Simulator::Schedule(
        MilliSeconds(1),
        &MonitorQueue,
        queue);
}


void RerouteToPathB(Ptr<Ipv4StaticRouting> routing, Ptr<Ipv4> routerIpv4)
{
    Ipv4Address pathBNextHop("10.1.7.2");
    Ipv4Address destinationNetwork("10.1.12.0");
    Ipv4Mask destinationMask("255.255.255.0");

    int32_t pathBInterface =
        routerIpv4->GetInterfaceForAddress(Ipv4Address("10.1.7.1"));

    if (pathBInterface < 0)
    {
        std::cout << "*** ROUTING ERROR: Path B interface not found ***\n";
        return;
    }

    routing->AddNetworkRouteTo(
        destinationNetwork,
        destinationMask,
        pathBNextHop,
        static_cast<uint32_t>(pathBInterface));

    std::cout << "\n*** PROACTIVE REROUTING: PATH A -> PATH B ***\n";
}


void RunEarlyMLDecision(
    Ptr<FlowMonitor> monitor,
    Ptr<Ipv4StaticRouting> routing,
    Ptr<Ipv4> routerIpv4,
    Ptr<Queue<Packet>> queue,
    double trafficRate)
{
    monitor->CheckForLostPackets();

    std::map<FlowId, FlowMonitor::FlowStats> stats =
        monitor->GetFlowStats();

    Ptr<Ipv4FlowClassifier> classifier =
        DynamicCast<Ipv4FlowClassifier>(
            FlowMonitorHelper().GetClassifier());

    if (stats.empty())
    {
        std::cout << "\n*** ML: No flow statistics available at 6 seconds ***\n";
        return;
    }

    auto flow = stats.begin()->second;

    double delay = 0.0;
    double jitter = 0.0;
    double throughput = 0.0;

    if (flow.rxPackets > 0)
    {
        delay =
            flow.delaySum.GetSeconds()
            / flow.rxPackets;
    }

    if (flow.rxPackets > 1)
    {
        jitter =
            flow.jitterSum.GetSeconds()
            / (flow.rxPackets - 1);
    }

    double elapsedTime = 6.0 - 2.0;

    if (flow.timeFirstRxPacket.GetSeconds() > 0)
    {
        elapsedTime =
            6.0 - flow.timeFirstTxPacket.GetSeconds();

        if (elapsedTime <= 0.0)
        {
            elapsedTime = 4.0;
        }
    }

    throughput =
        (flow.rxBytes * 8.0)
        / elapsedTime
        / 1000000.0;

    uint32_t currentQueue = queue->GetNPackets();

    double utilization =
        std::min(trafficRate / 2.0, 1.0) * 100.0;

    earlyPacketsSent = flow.txPackets;
    earlyPacketsReceived = flow.rxPackets;
    earlyPacketsLost = flow.lostPackets;
    earlyDelay = delay;
    earlyJitter = jitter;
    earlyThroughput = throughput;
    earlyQueue = currentQueue;
    earlyUtilization = utilization;

    std::cout << "\n========== EARLY ML CHECK @ 6s ==========\n";
    std::cout << "Packets Sent   : " << flow.txPackets << "\n";
    std::cout << "Packets Recv   : " << flow.rxPackets << "\n";
    std::cout << "Packets Lost   : " << flow.lostPackets << "\n";
    std::cout << "Average Delay  : " << delay << " seconds\n";
    std::cout << "Average Jitter : " << jitter << " seconds\n";
    std::cout << "Throughput     : " << throughput << " Mbps\n";
    std::cout << "Current Queue  : " << currentQueue << " packets\n";
    std::cout << "Utilization    : " << utilization << " %\n";

    std::string command =
        "python3 ml_decision.py "
        + std::to_string(trafficRate) + " "
        + std::to_string(flow.txPackets) + " "
        + std::to_string(flow.rxPackets) + " "
        + std::to_string(flow.lostPackets) + " "
        + std::to_string(delay) + " "
        + std::to_string(jitter) + " "
        + std::to_string(throughput) + " "
        + std::to_string(currentQueue) + " "
        + std::to_string(utilization)
        + " > ml_decision.txt";

    int result = system(command.c_str());

    if (result != 0)
    {
        std::cout << "*** ML ERROR: Python prediction failed ***\n";
        return;
    }

    std::ifstream file("ml_decision.txt");
    std::string line;
    bool reroute = false;

    while (std::getline(file, line))
    {
        std::cout << "ML: " << line << "\n";

        if (line.find("ACTION: REROUTE TO PATH B") != std::string::npos)
        {
            reroute = true;
        }
    }

    if (reroute)
    {
        Simulator::Schedule(
            Seconds(0.0),
            &RerouteToPathB,
            routing,
            routerIpv4);
    }
    else
    {
        std::cout << "*** ML: NO CONGESTION - KEEP CURRENT PATH ***\n";
    }

    std::cout << "==========================================\n";
}

int main(int argc, char *argv[])
{
    double trafficRate = 5.0;
    CommandLine cmd;
    cmd.AddValue("rate", "Traffic rate in Mbps", trafficRate);
    cmd.Parse(argc, argv);
    std::ofstream csv("ns3_dataset.csv", std::ios::app);

    // 1. CREATE NODES: source + 10 routers + destination
    NodeContainer nodes;
    nodes.Create(12);

    Ptr<Node> source = nodes.Get(0);
    Ptr<Node> router1 = nodes.Get(1);
    Ptr<Node> router2 = nodes.Get(2);
    Ptr<Node> router3 = nodes.Get(3);
    Ptr<Node> router4 = nodes.Get(4);
    Ptr<Node> router5 = nodes.Get(5);
    Ptr<Node> router6 = nodes.Get(6);
    Ptr<Node> router7 = nodes.Get(7);
    Ptr<Node> router8 = nodes.Get(8);
    Ptr<Node> router9 = nodes.Get(9);
    Ptr<Node> router10 = nodes.Get(10);
    Ptr<Node> destination = nodes.Get(11);

    PointToPointHelper fastLink;
    fastLink.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    fastLink.SetChannelAttribute("Delay", StringValue("2ms"));

    PointToPointHelper bottleneckA;
    bottleneckA.SetDeviceAttribute("DataRate", StringValue("2Mbps"));
    bottleneckA.SetChannelAttribute("Delay", StringValue("10ms"));

    PointToPointHelper pathB;
    pathB.SetDeviceAttribute("DataRate", StringValue("8Mbps"));
    pathB.SetChannelAttribute("Delay", StringValue("5ms"));

    // Shared entry link
    NetDeviceContainer d01 = fastLink.Install(source, router1);

    // Path A: R1 -> R2 -> R3 -> R4 -> R5 -> R9
    NetDeviceContainer d12 = bottleneckA.Install(router1, router2);
    NetDeviceContainer d23 = fastLink.Install(router2, router3);
    NetDeviceContainer d34 = fastLink.Install(router3, router4);
    NetDeviceContainer d45 = fastLink.Install(router4, router5);
    NetDeviceContainer d59 = fastLink.Install(router5, router9);

    // Path B: R1 -> R6 -> R7 -> R8 -> R10 -> R9
    NetDeviceContainer d16 = pathB.Install(router1, router6);
    NetDeviceContainer d67 = pathB.Install(router6, router7);
    NetDeviceContainer d78 = pathB.Install(router7, router8);
    NetDeviceContainer d810 = pathB.Install(router8, router10);
    NetDeviceContainer d109 = pathB.Install(router10, router9);

    // Shared exit link
    NetDeviceContainer d9d = fastLink.Install(router9, destination);

    // Monitor Path A bottleneck
    Ptr<PointToPointNetDevice> bottleneckDevice =
        DynamicCast<PointToPointNetDevice>(d12.Get(0));
    Ptr<Queue<Packet>> bottleneckQueue = bottleneckDevice->GetQueue();
    bottleneckDevice->TraceConnectWithoutContext(
        "PhyTxBegin", MakeCallback(&CountPathATransmission));

    Ptr<PointToPointNetDevice> pathBDevice =
        DynamicCast<PointToPointNetDevice>(d16.Get(0));
    pathBDevice->TraceConnectWithoutContext(
        "PhyTxBegin", MakeCallback(&CountPathBTransmission));

    InternetStackHelper internet;
    internet.Install(nodes);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0"); address.Assign(d01);
    address.SetBase("10.1.2.0", "255.255.255.0"); address.Assign(d12);
    address.SetBase("10.1.3.0", "255.255.255.0"); address.Assign(d23);
    address.SetBase("10.1.4.0", "255.255.255.0"); address.Assign(d34);
    address.SetBase("10.1.5.0", "255.255.255.0"); address.Assign(d45);
    address.SetBase("10.1.6.0", "255.255.255.0"); address.Assign(d59);
    address.SetBase("10.1.7.0", "255.255.255.0"); address.Assign(d16);
    address.SetBase("10.1.8.0", "255.255.255.0"); address.Assign(d67);
    address.SetBase("10.1.9.0", "255.255.255.0"); address.Assign(d78);
    address.SetBase("10.1.10.0", "255.255.255.0"); address.Assign(d810);
    address.SetBase("10.1.11.0", "255.255.255.0"); address.Assign(d109);
    address.SetBase("10.1.12.0", "255.255.255.0"); address.Assign(d9d);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    AnimationInterface anim("multipath.xml");
    anim.EnableIpv4L3ProtocolCounters(Seconds(0), Seconds(15));

    anim.SetConstantPosition(source, 2.0, 25.0);
    anim.SetConstantPosition(router1, 14.0, 25.0);

    // Path A - lower row
    anim.SetConstantPosition(router2, 26.0, 40.0);
    anim.SetConstantPosition(router3, 38.0, 40.0);
    anim.SetConstantPosition(router4, 50.0, 40.0);
    anim.SetConstantPosition(router5, 62.0, 40.0);

    // Path B - upper row
    anim.SetConstantPosition(router6, 26.0, 10.0);
    anim.SetConstantPosition(router7, 38.0, 10.0);
    anim.SetConstantPosition(router8, 50.0, 10.0);
    anim.SetConstantPosition(router10, 62.0, 10.0);

    // Shared exit
    anim.SetConstantPosition(router9, 74.0, 25.0);
    anim.SetConstantPosition(destination, 86.0, 25.0);

    anim.UpdateNodeDescription(source, "SOURCE");
    anim.UpdateNodeDescription(router1, "R1 - ENTRY");
    anim.UpdateNodeDescription(router2, "R2 - PATH A");
    anim.UpdateNodeDescription(router3, "R3 - PATH A");
    anim.UpdateNodeDescription(router4, "R4 - PATH A");
    anim.UpdateNodeDescription(router5, "R5 - PATH A");
    anim.UpdateNodeDescription(router6, "R6 - PATH B");
    anim.UpdateNodeDescription(router7, "R7 - PATH B");
    anim.UpdateNodeDescription(router8, "R8 - PATH B");
    anim.UpdateNodeDescription(router9, "R9 - EXIT");
    anim.UpdateNodeDescription(router10, "R10 - PATH B");
    anim.UpdateNodeDescription(destination, "DESTINATION");

    Ptr<Ipv4> router1Ipv4 = router1->GetObject<Ipv4>();
    Ptr<Ipv4StaticRouting> staticRouting =
        Ipv4StaticRoutingHelper().GetStaticRouting(router1Ipv4);

    // =========================================================
    // 7. CREATE UDP SERVER
    // =========================================================

    uint16_t port = 5000;

    PacketSinkHelper sink(
        "ns3::UdpSocketFactory",
        InetSocketAddress(Ipv4Address::GetAny(), port));

    ApplicationContainer sinkApp =
        sink.Install(destination);

    sinkApp.Start(Seconds(1.0));
    sinkApp.Stop(Seconds(15.0));

    // =========================================================
    // 8. CREATE TRAFFIC SOURCE
    // =========================================================

    // Destination IP is taken from destination node
    Ptr<Ipv4> ipv4Destination =
        destination->GetObject<Ipv4>();

    Ipv4Address destinationAddress =
        ipv4Destination->GetAddress(1, 0).GetLocal();

    OnOffHelper sourceApp(
        "ns3::UdpSocketFactory",
        InetSocketAddress(destinationAddress, port));

    // Traffic higher than Path-A bottleneck
    sourceApp.SetAttribute(
        "DataRate",
        StringValue(std::to_string(trafficRate) + "Mbps"));

    sourceApp.SetAttribute(
        "PacketSize",
        UintegerValue(1024));

    sourceApp.SetAttribute(
        "OnTime",
        StringValue(
            "ns3::ConstantRandomVariable[Constant=1]"));

    sourceApp.SetAttribute(
        "OffTime",
        StringValue(
            "ns3::ConstantRandomVariable[Constant=0]"));

    ApplicationContainer traffic =
        sourceApp.Install(source);

    traffic.Start(Seconds(2.0));
    traffic.Stop(Seconds(14.0));

    // =========================================================
    // 9. FLOW MONITOR
    // =========================================================

    FlowMonitorHelper flowmon;

    Ptr<FlowMonitor> monitor =
        flowmon.InstallAll();

    std::cout << "\n========================================\n";
    std::cout << "   MULTI-PATH NETWORK SIMULATION\n";
    std::cout << "========================================\n";
    std::cout << "Path A Bottleneck : 2 Mbps\n";
    std::cout << "Path B Capacity   : 8 Mbps\n";
    std::cout << "Traffic Rate      : " << trafficRate << " Mbps\n";
    std::cout << "========================================\n";
    std::cout << "Simulation started...\n";

    Simulator::Schedule(
        MilliSeconds(1),
        &MonitorQueue,
        bottleneckQueue);

    Simulator::Schedule(
        MilliSeconds(1),
        &RecordNetworkTrace,
        bottleneckQueue);

    Simulator::Schedule(
        Seconds(6.0),
        &RunEarlyMLDecision,
        monitor,
        staticRouting,
        router1Ipv4,
        bottleneckQueue,
        trafficRate);

    Simulator::Stop(Seconds(15.0));
    Simulator::Run();

    // =========================================================
    // 10. COLLECT RESULTS
    // =========================================================

    monitor->CheckForLostPackets();

    Ptr<Ipv4FlowClassifier> classifier =
        DynamicCast<Ipv4FlowClassifier>(
            flowmon.GetClassifier());

    std::map<FlowId, FlowMonitor::FlowStats> stats =
        monitor->GetFlowStats();

    for (const auto& flow : stats)
    {
        Ipv4FlowClassifier::FiveTuple t =
            classifier->FindFlow(flow.first);

        std::cout << "\n----------------------------------------\n";
        std::cout << "Flow ID       : " << flow.first << "\n";
        std::cout << "Source        : " << t.sourceAddress << "\n";
        std::cout << "Destination   : " << t.destinationAddress << "\n";
        std::cout << "Packets Sent  : "
                  << flow.second.txPackets << "\n";
        std::cout << "Packets Recv  : "
                  << flow.second.rxPackets << "\n";
        std::cout << "Packets Lost  : "
                  << flow.second.lostPackets << "\n";

    std::cout << "Path A Bytes  : "
              << pathABytesTransmitted << "\n";

    std::cout << "Path B Bytes  : "
              << pathBBytesTransmitted << "\n";

        if (flow.second.rxPackets > 0)
        {
            double delay =
                flow.second.delaySum.GetSeconds()
                / flow.second.rxPackets;

            double jitter = 0.0;
            if (flow.second.rxPackets > 1)
            {
                jitter =
                    flow.second.jitterSum.GetSeconds()
                    / (flow.second.rxPackets - 1);
            }

            std::cout << "Average Delay : "
                      << delay << " seconds\n";
            std::cout << "Average Jitter: "
                      << jitter << " seconds\n";
        }

        if (flow.second.rxBytes > 0)
        {
            double duration =
                flow.second.timeLastRxPacket.GetSeconds()
                - flow.second.timeFirstRxPacket.GetSeconds();

            if (duration > 0)
            {
                double throughput =
                    flow.second.rxBytes * 8.0
                    / duration
                    / 1000000.0;

                std::cout << "Throughput    : "
                          << throughput << " Mbps\n";

            double lossPercent = (flow.second.txPackets > 0) ? (100.0 * flow.second.lostPackets / flow.second.txPackets) : 0.0;
            std::string congestionClass = (lossPercent > 1.0 || (flow.second.rxPackets > 0 && (flow.second.delaySum.GetSeconds() / flow.second.rxPackets) > 0.05)) ? "CONGESTION" : "NO_CONGESTION";
            double csvJitter = 0.0;
            if (flow.second.rxPackets > 1)
            {
                csvJitter =
                    flow.second.jitterSum.GetSeconds()
                    / (flow.second.rxPackets - 1);
            }

            double pathAUtilizationCsv =
                std::min(trafficRate / 2.0, 1.0) * 100.0;

            csv << trafficRate << ","
                << flow.second.txPackets << ","
                << flow.second.rxPackets << ","
                << flow.second.lostPackets << ","
                << (flow.second.delaySum.GetSeconds() / flow.second.rxPackets) << ","
                << csvJitter << ","
                << throughput << ","
                << maxQueueSize << ","
                << pathAUtilizationCsv << ","
                << congestionClass << "\n";

            std::ofstream earlyCsv(
                "early_ml_dataset.csv",
                std::ios::app);

            earlyCsv << trafficRate << ","
                     << earlyPacketsSent << ","
                     << earlyPacketsReceived << ","
                     << earlyPacketsLost << ","
                     << earlyDelay << ","
                     << earlyJitter << ","
                     << earlyThroughput << ","
                     << earlyQueue << ","
                     << earlyUtilization << ","
                     << congestionClass << "\n";

            earlyCsv.close();
            }
        }
    }

    std::cout << "\n----------------------------------------\n";
    std::cout << "Path A Current Queue : "
              << bottleneckQueue->GetNPackets()
              << " packets\n";
    std::cout << "Path A Maximum Queue : "
              << maxQueueSize
              << " packets\n";

    double offeredLoad = trafficRate / 2.0;
    double pathAUtilization = std::min(offeredLoad, 1.0) * 100.0;

    std::cout << "Path A Link Utilization : "
              << pathAUtilization
              << " %\n";


    std::cout << "\n========================================\n";
    std::cout << "Simulation completed.\n";
    std::cout << "========================================\n";

    Simulator::Destroy();

    return 0;
}
