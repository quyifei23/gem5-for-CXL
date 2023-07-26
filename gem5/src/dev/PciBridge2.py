from m5.params import *
from m5.SimObject import *
from m5.proxy import *
from m5.objects.PciHost import GenericPciHost
from m5.objects.Platform import Platform


class PcPciHost(GenericPciHost):
    conf_base = 0xC000000000000000
    conf_size = "16MiB"

    pci_pio_base = 0x8000000000000000
class PciBridge2(SimObject):
    type = 'PciBridge2'
    #abstract = True
    cxx_class = 'gem5::PciBridge2' 
    cxx_header = 'dev/PciBridge2.hh'
    rc_id = Param.UInt8(0 , 'Root Complex Id')
    is_switch = Param.Int(0 , 'Whether this is a switch and virtual p2p bridge needed for upstream port')
    is_transmit = Param.Int(0, 'Should this root port count bytes')
    PrimaryBusNumber = Param.UInt8(0x00 , "Upstream bus of PCI-PCI bridge")
    SecondaryBusNumber = Param.UInt8(0x00 , "Downstream Bus of PCI-PCI bridge")
    SubordinateBusNumber = Param.UInt8(0x00 , "LArgest bus number downstream of bridge") 
    pci_bus = Param.Int("PCI bus")
    pci_upstream_dev = Param.Int( 0  ,"PCI device number of upstream switch port")
    pci_upstream_func = Param.Int(0 , "PCI func number of upstream switch port")
    pci_dev1 = Param.Int("PCI device number")
    pci_func1 =Param.Int("PCI function code")
    pci_dev2 = Param.Int("PCI device number")
    pci_func2 = Param.Int("PCI func number")
    pci_dev3  = Param.Int("PCi dev")
    pci_func3 = Param.Int("PCI function") 
    host = Param.PciHost(Parent.any, "PCI host")
    DeviceId1 = Param.UInt16("Bridge's device id") # For Root port 1
    DeviceId2 = Param.UInt16("Bridge's device id") #For Root port 2
    DeviceId3 = Param.UInt16("Bridge's device id") #For Root port 3
    DeviceId_upstream = Param.UInt16(0x8232 , "P2P Bridge's device Id") # For upstream switch port 
    VendorId = Param.UInt16("Bridge's vendor id")
    Status   = Param.UInt16(0 , "status") 
    Command  = Param.UInt16(0 , "command")
    ClassCode = Param.UInt8(0 , "class code")
    SubClassCode = Param.UInt8(0, "subclass code")
    ProgIF    = Param.UInt8(0 , "Programming Interface byte")
 
    Revision  = Param.UInt8(0 , "revision")
    BIST      = Param.UInt8(0 , "Built in self test")
    HeaderType = Param.UInt8(1 , "Header type for PCI bridge")
    LatencyTimer = Param.UInt8(0 , "Latency timer")
    CacheLineSize = Param.UInt8(0, "System Cache Line size")
    Bar0  =  Param.UInt32(0x00000000 , "Base Address 0 reg value")
    Bar1  =  Param.UInt32(0x00000000 , "Base Address 1 reg value")
    BAR0Size = Param.MemorySize32('0B', "Base Address Register 0 Size")
    BAR1Size = Param.MemorySize32('0B', "Base Address Register 1 Size")
    SecondaryLatencyTimer = Param.UInt8(0, "Secondary LAtency Timer")
    MemoryLimit = Param.UInt16(0x0000 , "Memory limit for devices downstream of bridge")
    MemoryBase  = Param.UInt16(0x0000 , "Memory Base foe devices downstream of Bridge")
    PrefetchableMemoryLimit = Param.UInt16(0x0000 , "Prefetchable memory limit")
    PrefetchableMemoryBase  = Param.UInt16(0x0000 , "Prefetchable memory base")
    PrefetchableMemoryLimitUpper = Param.UInt32(0x00000000 , "Upper 32 bits of prefetchable memory limit")
    PrefetchableMemoryBaseUpper  = Param.UInt32(0x00000000 , "Upper 32 bits of prefetchable memory base")
    IOLimit = Param.UInt8(0x01 , "Lower bits of IO limit")
    IOBase  = Param.UInt8(0x01 , "lower limits of IO Base")
    IOLimitUpper = Param.UInt16(0x2f00 , "Upper 16 bits of IO limit")
    IOBaseUpper  = Param.UInt16(0x2f00 , "Upper 16 bits of IO Base")
    CapabilityPointer = Param.UInt8(0x00 , "Pointer to extended capabilities if any")
    ExpansionROMBaseAddress = Param.UInt32(0 , " Base address for ROM if it exists")
    InterruptPin = Param.UInt8(0 , "PCI interrupt pin")
    InterruptLine = Param.UInt8(0 , "Interrupt line used in inteerupt controller")
    BridgeControl = Param.UInt16( 0 , "Bridge control register")
    SecondaryStatus = Param.UInt16(0 , "Bridge's secondary interface status register")
###PCIe capability fields (8 datawords for PCI Bridge)
    PXCAPBaseOffset = \
        Param.UInt8(0x00, "Base offset of PXCAP in PCI Config space")
    PXCAPNextCapability = Param.UInt8(0x00, "Pointer to next capability block")
    PXCAPCapId = Param.UInt8(0x00, "Specifies this is the PCIe Capability")
    PXCAPCapabilities = Param.UInt16(0x0000, "PCIe Capabilities")
    PXCAPDevCapabilities = Param.UInt32(0x00000000, "PCIe Device Capabilities")
    PXCAPCapabilities_downstream = Param.UInt16(0x0061, "PCIe Capabilities for downstream switch port")
    PXCAPCapabilities_upstream    = Param.UInt16(0x0051, "PCIe Capabilities for upstream switch port")
    PXCAPDevCtrl = Param.UInt16(0x0000, "PCIe Device Control")
    PXCAPDevStatus = Param.UInt16(0x0000, "PCIe Device Status")
    PXCAPLinkCap = Param.UInt32(0x00000000, "PCIe Link Capabilities")
    PXCAPLinkCtrl = Param.UInt16(0x0000, "PCIe Link Control")
    PXCAPLinkStatus = Param.UInt16(0x0000, "PCIe Link Status")
    PXCAPDevCap2 = Param.UInt32(0x00000000, "PCIe Device Capabilities 2")
    PXCAPDevCtrl2 = Param.UInt32(0x00000000, "PCIe Device Control 2")
    PXCAPRootStatus = Param.UInt32(0x00000000 , "Root port status reg")
    PXCAPRootControl = Param.UInt16(0x0000, "Root port ctrl reg")


###Bridge related params
    response = ResponsePort('Response port')
    request1 = RequestPort('Request port')
    request2 = RequestPort('Request port') 
    request3 = RequestPort('Request Port')

    response_dma1 = ResponsePort('Response port')
    request_dma = RequestPort('Request port')
    response_dma2 = ResponsePort('Response port') 
    response_dma3 = ResponsePort('Response Port ') 
    
    req_size = Param.Unsigned(16, "The number of requests to buffer")
    resp_size = Param.Unsigned(16, "The number of responses to buffer")
    delay = Param.Latency('0ns', "The latency of this bridge")
    config_latency = Param.Latency('20ns', "Config read or write latency")

class RootComplex(PciBridge2) :

    PrimaryBusNumber = 0x00 
    SecondaryBusNumber= 0x00
    SubordinateBusNumber = 0x00
    pci_bus = 0
    pci_dev1 = 4
    pci_dev2 = 5 
    pci_dev3 = 6 
    pci_func1 = 0
    pci_func2 = 0 
    pci_func3 = 0 

    VendorId = 0x8086
    DeviceId1 = 0x9c90  # intel pci express root port 1
    DeviceId2 = 0x9c92  # intel pci express root port 2
    DeviceId3 = 0x9c94  #intel pci express root port 3 
    ClassCode = 0x06 
    SubClassCode = 0x04
    ProgIF = 0x00
    HeaderType = 0x01
    Revision = 0x00
    BIST = 0x00
    Command = 0x0407 # Enable bus mastering, intx gen,memory address space decoder, io address space decoder
    Status = 0x0010   #Indicate that capability ptr has been implemented
    SecondaryStatus = 0x0000
    BridgeControl = 0x0000
    InterruptPin = 0x01
    InterruptLine = 0x20
    CapabilityPointer = 0xD0
    PXCAPBaseOffset = 0xD0
    PXCAPNextCapability = 0x00 #implement only PCIe capability
    PXCAPCapId = 0x10 #PCIe capability
    PXCAPCapabilities = 0x0041  #4 indicates PCIe root port
    PXCAPDevCapabilities = 0x00000241  # Max payload 256B , L0 acceptable latency: 64-128ns. L1 acceptable latency 1-2 us
    PXCAPDevCtrl =  0x1020 # Max payload size : 256B. Max read request size : 256B
    PXCAPLinkCap =0x01009011 # Max link width : 6'b1 , Max link speed : 2.5 Gbps , ASPM:00 , L0s exit latency: 64-128 ns , L1 exit latency 1-2 us. Port number : 1. 
    PXCAPDevStatus =  0x0000
    PXCAPLinkCtrl  = 0x0008 # Root ports:128 B Read Completion Boundary(RCB) support , ASPM disabled=> Bits 1 and 0 are just 0 . 
    PXCAPLinkStatus =  0x0011 #Negotiated link width : 1 , Negotiated link speed : 2.5 Gbps

# Need to implement (slot??) registers and root registers for a root port. Assume that drivers are not needed for Root Port detection and Configuration (Assume that kernel bus drivers do this). 
#Also need to assign bridge params to the PciBridge. So , master-port, slave-port and latencies need to be assigned. Assume bridges can raise interrupts (not required ??).  
    PXCAPRootStatus = 0x00000000 #PME related stuff. Ignore for now
    PXCAPRootControl = 0x0000 #PME related stuff again. Ignore for now. 
    


class PCIESwitch(PciBridge2):
 
    
    is_switch = 1
    pci_bus = 1
    pci_dev1 =  0
    pci_func1 = 0 
    pci_dev2 =  1 
    pci_func2 = 0
    pci_dev3  = 2
    pci_func3 = 0  
    DeviceId1 = 0x8233 
    DeviceId2 = 0x8233 
    DeviceId3 = 0x8233  
      
    VendorId = 0x104c                           # Vendor is texas instruments
    Status   = 0x0010  
    Command  = 0x0407 
    ClassCode = 0x06 
    SubClassCode = 0x04
    ProgIF    = 0 
 
    Revision  = 0 
    BIST      = 0 
    HeaderType = 0x01 
    LatencyTimer = 0 
    CacheLineSize = 0
    Bar0  =  0x00000000 
    Bar1  =  0x00000000 
    BAR0Size = '0B'
    BAR1Size = '0B'
    SecondaryLatencyTimer = 0
    
    CapabilityPointer = 0xD0 
    InterruptPin =      0x01 
    InterruptLine =     0x20
    BridgeControl =     0x0000 
    SecondaryStatus =   0x0000 
###PCIe capability fields (8 datawords for PCI Bridge)
    PXCAPBaseOffset =   0xD0
    PXCAPNextCapability = 0x00
    PXCAPCapId =         0x10
   

    PXCAPDevCtrl =       0x1020
    PXCAPDevStatus =     0x0000 
    PXCAPLinkCap =       0x01009011
    PXCAPLinkCtrl =      0x0008 
    PXCAPLinkStatus =    0x0011
    PXCAPDevCap2 =       0x00000000
    PXCAPDevCtrl2 =      0x00000000 
    PXCAPRootStatus =    0x00000000 
    PXCAPRootControl =   0x0000

 
