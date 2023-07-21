#ifndef __MEM_PCIBRIDGE_HH__
#define __MEM_PCIBRIDGE_HH__

#include <deque>
#include "base/types.hh"
#include "params/PciBridge.hh"
#include "mem/port.hh"
#include "sim/sim_object.hh"

namespace gem5
{
    class PciBridge : public SimObject
    {
        /**
         * A deferred packet stores a packet along with its scheduled
         * transmission time
         */
        // class DeferredPacket
        // {
        //     public:
        //         const Tick tick;
        //         const PacketPtr pkt;
        //         DeferredPacket(PacketPtr _pkt,Tick _tick) : tick(_tick),pkt(_pkt){}
        // };
        class PciBridgeReceivePort;
        class PciBridgeSendPort;
        class PciBridgeReceivePort : public RequestPort
        {
            private:
                PciBridge& bridge;
                // const Tick delay;
                // /**
                //  * Response packet queue. Response packets are held in this
                //  * queue for a specified delay to model the processing delay
                //  * of the bridge. We use a deque as we need to iterate over
                //  * the items for functional accesses.
                //  */
                // std::deque<DeferredPacket> transmitList;

                // unsigned int outstandingResponse;
                // bool retryReq;
                // bool is_upstream;

                // unsigned int respDequeLimit;
                // std::unique_ptr<Packet> pendingDelete;
                // bool respDequeFull() const;
                // void trySendTiming();

                // EventFunctionWrapper sendEvent;
            public:
                PciBridgeReceivePort(const std::string& _name,PciBridge& _bridge, uint8_t receiveID)
                uint8_t receiveID;
            protected:
                /** When receiving a timing request from the peer port,
                    pass it to the bridge. */
                bool recvTimingReq(PacketPtr pkt);

                /** When receiving a retry request from the peer port,
                    pass it to the bridge. */
                void recvRespRetry();

                /** When receiving a Atomic requestfrom the peer port,
                    pass it to the bridge. */
                Tick recvAtomic(PacketPtr pkt);

                /** When receiving a Functional request from the peer port,
                    pass it to the bridge. */
                void recvFunctional(PacketPtr pkt);

                /** When receiving a address range request the peer port,
                    pass it to the bridge. */
                AddrRangeList getAddrRanges() const;
                

        }
        class PciBridgeSendPort : public RequestPort
        {
            private:
                PciBridge& bridge;
            public:
                PciBridgeSendPort(const std::string& _name,PciBridge& _bridge)
        }
        public:
            PciBridge(const PciBridgeParams *params);
        
        protected:

            /** When receiving a timing request from the peer port,
                pass it to the bridge. */
            bool recvTimingResp(PacketPtr pkt);

            /** When receiving a retry request from the peer port,
                pass it to the bridge. */
            void recvReqRetry();
    }
    public:
        PciBridgeSendPort SendUpPort;
        PciBridgeReceivePort ReceiveUpPort;
        PciBridgeSendPort SendDownPort1;
        PciBridgeReceivePort ReceiveDownPort1;
        PciBridgeSendPort SendDownPort2;
        PciBridgeReceivePort ReceiveDownPort2;
        PciBridgeSendPort SendDownPor3;
        PciBridgeReceivePort ReceiveDownPort3;
        PciBridge(const PciBridgeParams *p);
}

#endif
