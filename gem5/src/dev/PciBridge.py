from m5.SimObject import SimObject
from m5.params import *

class PciBridge(SimObject):
    type = 'PciBridge'
    cxx_class = 'gem5::PciBridge'
    cxx_header = 'dev/PciBridge.hh'
    
    receiveUpPort = ResponsePort("receive port up")
    receiveDownPort1 = ResponsePort("receive port1")
    receiveDownPort2 = ResponsePort("receive port2")
    receiveDownPort3 = ResponsePort("receive port3")
    
    sendUpPort = RequestPort("send port up")
    sendDownPort1 = RequestPort("send port1")
    sendDownPort2 = RequestPort("send port2")
    sendDownPort3 = RequestPort("send port3")