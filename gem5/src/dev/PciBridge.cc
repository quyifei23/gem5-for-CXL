#include "debug/PciBridge.hh"
#include "params/PciBridge.hh"
#include "mem/packet.hh"
#include "mem/packet_access.hh"
#include "dev/PciBridge.hh"

namespace gem5
{
    PciBridge::PciBridgeReceivePort::PciBridgeReceivePort(const std::string,PciBridge& _bridge,uint8_t _receiveID)
    : ReponsePort(_name,&_bridge),bridge(_bridge),receiveID(_receiveID)
    {}


    PciBridge::PciBridgeSendPort::PciBridgeSendPort(const std::string,PciBridge& _bridge)
    : RequestPort(_name,&_bridge),bridge(_bridge)
    {}


    PciBridge::PciBridge(const PciBridgeParams *p)
    : SimObject(p),
    receiveUpPort(p->name + ".receiveUpPort",*this,0),
    receiveDownPort(p->name + ".receiveDownPort1",*this,1),
    receiveDownPort(p->name + ".receiveDownPort2",*this,2),
    receiveDownPort(p->name + ".receiveDownPort3",*this,3),
    sendUpPort(p->name + ".sendUpPort",*this)
    sendDownPort(p->name + ".sendDownPort1",*this)
    sendDownPort(p->name + ".sendDownPort2",*this)
    sendDownPort(p->name + ".sendDownPort3",*this)
    {
        DPRINTF(PciBridge,"create the PciBridge\n");
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

    void PciBridge::PciBridgeReceivePort::recvFunctional(Packet pkt)
    {

    } 

    AddrRangeList PciBridge::PciBridgeReceivePort::getAddrRanges() const
    {
        AddrRangeList ranges;
        return ranges;
    }
}