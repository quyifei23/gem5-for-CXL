import m5
from m5.objects import *

# create a system object
system=System()

# set the clock 
system.clk_domain=SrcClockDomain()
system.clk_domain.clock='1GHz'
system.clk_domain.voltage_domain=VoltageDomain()

# simulate the memory
system.mem_mode='timing'
system.mem_ranges=[AddrRange('512MB')]

# create the CPU
system.cpu=X86TimingSimpleCPU()

# create a memory bus
system.membus=SystemXBar()

# connect the cache on CPU
system.cpu.icache_port=system.membus.cpu_side_ports
system.cpu.dcache_port=system.membus.cpu_side_ports

# connect the request ports with the respond ports
#system.cpu.icache_port=system.l1_cache.cpu_side

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