#include "debug/PciBridge.hh"
#include "mem/packet.hh"
#include "mem/packet_access.hh"
#include "dev/PciBridge.hh"

namespace gem5
{
    PciBridge::PciBridgeReceivePort::PciBridgeReceivePort(const std::string& _name,PciBridge& _bridge,uint8_t _receiveID)
    : ResponsePort(_name,&_bridge),bridge(_bridge),receiveID(_receiveID)
    {}


    PciBridge::PciBridgeSendPort::PciBridgeSendPort(const std::string& _name,PciBridge& _bridge)
    : RequestPort(_name,&_bridge),bridge(_bridge)
    {}


    PciBridge::PciBridge(const PciBridgeParams & p)
    : SimObject(p),
    sendUpPort(p.name + ".sendUpPort",*this),
    receiveUpPort(p.name + ".receiveUpPort",*this,0),
    sendDownPort1(p.name + ".sendDownPort1",*this),
    receiveDownPort1(p.name + ".receiveDownPort1",*this,1),
    sendDownPort2(p.name + ".sendDownPort2",*this),
    receiveDownPort2(p.name + ".receiveDownPort2",*this,2),
    sendDownPort3(p.name + ".sendDownPort3",*this),
    receiveDownPort3(p.name + ".receiveDownPort3",*this,3)
    {
        DPRINTF(PciBridge,"create the PciBridge\n");
    }
    PciBridge::~PciBridge()
    {

    }
    bool PciBridge::PciBridgeSendPort::recvTimingResp(PacketPtr pkt)
    {
        return true;
    }

    bool PciBridge::PciBridgeReceivePort::recvTimingReq(PacketPtr pkt)
    {
        return true;
    }

    void PciBridge::PciBridgeSendPort::recvReqRetry()
    {

    }
    void PciBridge::PciBridgeReceivePort::recvRespRetry()
    {

    }

    Tick PciBridge::PciBridgeReceivePort::recvAtomic(PacketPtr pkt)
    {
        return 0;
    }

    void PciBridge::PciBridgeReceivePort::recvFunctional(PacketPtr pkt)
    {

    } 

    AddrRangeList PciBridge::PciBridgeReceivePort::getAddrRanges() const
    {
        AddrRangeList ranges;
        return ranges;
    }
}