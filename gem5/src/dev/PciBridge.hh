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
        class PciBridgeSendPort : public RequestPort
        {
            private:
                PciBridge& bridge;


        }
        public:
            PciBridge(const PciBridgeParams *params);

    }
}

#endif
