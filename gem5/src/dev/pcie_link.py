from m5.params import *
from m5.SimObject import *

class PCIELink(SimObject):
    type = 'PCIELink'
    cxx_class = 'gem5::PCIELink'
    cxx_header = 'dev/pcie_link.hh'

    speed = Param.Float("speed")
    delay = Param.Tick(0,"delay")
    delay_var = Param.Tick(0,"delay_var")
    lanes = Param.Int("lanes")
    max_queue_size = Param.Int("max_queue_siz")
    mps = Param.Int("mps")
    upstreamResponse= ResponsePort("upstreamResponse")
    downstreamResponse = ResponsePort("downstreamResponse")
    upstreamRequest = RequestPort("upstreamRequest")
    downstreamRequest = RequestPort("downstreamRequest")