import m5
from m5.objects import *
from caches import *

from optparse import OptionParser
parser=OptionParser()
parser.add_option("--clk",help="CPU clock. Default: 1GHz")
parser.add_option("--l1i_size",help="L1 instruction cache size. Default: 16kB.")
parser.add_option("--l1d_size",help="L1 data cache size. Default: Default: 64kB.")
parser.add_option("--l2_size",help="L2 cache size. Default: 256kB.")
(options,args) = parser.parse_args()


# create a system object
system=System()

# set the clock 
system.clk_domain=SrcClockDomain()
if not options.clk:
    system.clk_domain.clock='1GHz'
else:
    system.clk_domain.clock = options.clk
system.clk_domain.voltage_domain=VoltageDomain()

# simulate the memory
system.mem_mode='timing'
system.mem_ranges=[AddrRange('512MB')]

# create the CPU
system.cpu=X86TimingSimpleCPU()

# create a memory bus
system.membus=SystemXBar()

# create L1 caches
system.cpu.icache=L1ICache(options)
system.cpu.dcache=L1DCache(options)

# connect the cache on CPU
system.cpu.icache.connectCPU(system.cpu)
system.cpu.dcache.connectCPU(system.cpu)

# connect the request ports with the respond ports
#system.cpu.icache_port=system.l1_cache.cpu_side

# create a L2 cache bus
system.l2bus=L2XBar()
system.cpu.icache.connectBus(system.l2bus)
system.cpu.dcache.connectBus(system.l2bus)

# create L2 cache and connect to L2 bus and memory bus
system.l2cache=L2Cache()
system.l2cache.connectCPUSideBus(system.l2bus)
system.l2cache.connectMemSideBus(system.membus)

# create an I/O controller
system.cpu.createInterruptController()
system.cpu.interrupts[0].pio=system.membus.mem_side_ports
system.cpu.interrupts[0].int_requestor=system.membus.cpu_side_ports
system.cpu.interrupts[0].int_responder=system.membus.mem_side_ports
system.system_port=system.membus.cpu_side_ports

# create a memory controller
system.mem_ctrl=MemCtrl()
system.mem_ctrl.dram=DDR3_1600_8x8()
system.mem_ctrl.dram.range=system.mem_ranges[0]
system.mem_ctrl.port=system.membus.mem_side_ports


# create the process
binary='tests/test-progs/hello/bin/x86/linux/hello'
system.workload=SEWorkload.init_compatible(binary)

process=Process()
process.cmd=[binary]
system.cpu.workload=process
system.cpu.createThreads()

root=Root(full_system=False,system=system)
m5.instantiate()
print("Beginning sumulation!")
exit_event=m5.simulate()
print('Exiting @ tick {} because {}'
      .format(m5.curTick(), exit_event.getCause()))