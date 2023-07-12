# CxlMemory设备代码解读

## CXL设备功能概览

一方面，CXL协议与PCIe协议共用物理层，所以在物理连接方式上，CXL设备是直接插在PCIe插槽上来接入计算机系统的，CXL.io子协议扮演了原本PCIe协议的角色，承担了CXL设备的枚举与配置等工作。所以对于CXL扩展内存设备而言，**需要设置设备的配置空间，包括设备的基本信息、BAR空间等，同时能够响应主机对配置空间的读写请求。**

> 响应主机对配置空间的读写请求：通过对PciDevice的继承，实现getAddrRanges()函数即可
>
> 设备的基本信息、BAR空间等：通过在CXLMemory.py文件中设置，同时与驱动程序约定相同即可

另一方面，CXL扩展内存设备拥有**自己的存储空间，需要实现读写功能，同时能够解析cxl协议的数据包**，进行响应。

> 自己的存储空间：通过实现一个Memory嵌套类，子类成员变量包括存储空间地址范围、指向存储地址的指针；成员函数主要包括access()访问函数，以根据不同的命令访问存储空间
>
> 读写功能：通过对PciDevice的继承，实现read()和write()函数，在read()和write()内部会调用access()访问存储空间
>
> 解析cxl协议的数据包：主要是在这里确定数据包的格式并模拟一个解析的延迟返回回去

## CxlMemory.py

在python层控制相关参数的具体赋值

```python
# CxlMemory对象自己独有的参数，在cxl_memory.hh声明
latency = Param.Latency('170ns', "cxl-memory device's latency for mem access")
cxl_mem_latency = Param.Latency('2ns', "cxl.mem protocol processing's latency for device")
```

```python
# 父类PciDevice的参数，与驱动程序约定的配置空间参数、BAR空间
VendorID = 0x8086
DeviceID = 0X7890
Command = 0x0
# Status = 0x280
Revision = 0x0
ClassCode = 0x01
SubClassCode = 0x01
ProgIF = 0x85
InterruptLine = 0x1f
InterruptPin = 0x01

# Primary
BAR0 = PciMemBar(size='4GiB')
BAR1 = PciMemUpperBar()
```

## cxl_memory.cc

```c++
CxlMemory::CxlMemory(const Param &p)
    : PciDevice(p),
    mem_(RangeSize(p.BAR0->addr(), p.BAR0->size()), *this),
    latency_(p.latency),
    cxl_mem_latency_(p.cxl_mem_latency) {}
```

CxlMemory对象的构造函数。

初始化自己的三个成员变量：

- `latency_`与`cxl_mem_latency_`是python层传入的；

- `mem_`是嵌套类对象，`RangeSize(p.BAR0->addr(), p.BAR0->size())`是BAR空间设定的大小（4GB）。

  > 注意在CXLMemory对象初始化时，也就是在`**** REAL SIMULATION ****`之前，`BAR0->addr()`应该是0，`BAR0->size()`是4GB，只有在开始仿真之后，主机不断的枚举并配置设备，重新给CXL设备存储空间分配一个地址范围，并写入到BAR0中。所以配置完成之后，BAR0中存的应该是`(0x100000000,0x200000000)也就是(4GB,8GB)`

- `mem_`的`*this`成员变量是指向CxlMemory外层类的指针，用于设备配置完成后读取BAR0更新range变量。

---------

```c++
Tick CxlMemory::read(PacketPtr pkt) {
    DPRINTF(CxlMemory, "read address : (%lx, %lx)\n", pkt->getAddr(),
            pkt->getSize());
    Tick cxl_latency = resolve_cxl_mem(pkt);
    mem_.access(pkt);
    DPRINTF(CxlMemory, "read latency = %llu\n", latency_ + cxl_latency);
    return latency_ + cxl_latency;
}

Tick CxlMemory::write(PacketPtr pkt) {
    DPRINTF(CxlMemory, "write address : (%lx, %lx)\n", pkt->getAddr(),
            pkt->getSize());
    Tick cxl_latency = resolve_cxl_mem(pkt);
    mem_.access(pkt);
    DPRINTF(CxlMemory, "write latency = %llu\n", latency_ + cxl_latency);
    return latency_ + cxl_latency;
}
```

重写的两个读写函数

- `Tick cxl_latency = resolve_cxl_mem(pkt);`：解析cxl协议的数据包，并返回一个解析的延迟
- `mem_.access(pkt);`：调用`mem_`对象的`access()`函数进行实质的访存

----------

```c++
Tick CxlMemory::resolve_cxl_mem(PacketPtr pkt) {
    if (pkt->cmd == MemCmd::M2SReq) {
        assert(pkt->isRead());
        assert(pkt->needsResponse());
    } else if (pkt->cmd == MemCmd::M2SRwD) {
        assert(pkt->isWrite());
        assert(pkt->needsResponse());
    }
    return cxl_mem_latency_;
}
```

主要就是解析一下pkt的命令是不是cxl.mem规定的`M2SReq`和`M2SRwD`

（之前凡丰师兄预想的应该会更复杂一点，会用到`src/mem/protocol/cxl_mem.hh`里的东西，但是目前就只实现到这一步）

-----------------

```c++
CxlMemory::Memory::Memory(const AddrRange& range, CxlMemory& owner)
    : range(range),
    owner(owner) {
    pmemAddr = new uint8_t[range.size()];
    DPRINTF(CxlMemory, "initial range start=0x%lx, range size=0x%lx\n", range.start(), range.size());
}
```

嵌套类Memory的构造函数

- `pmemAddr = new uint8_t[range.size()];`：直接`new`一段`range.size()`大小的空间作为cxl设备存储空间

  > 这里的`range.size()`就是CxlMemory初始化时传入的`RangeSize(p.BAR0->addr(), p.BAR0->size())`

-------------

```c++
void CxlMemory::Memory::access(PacketPtr pkt) {
    PciBar *bar = owner.BARs[0];
    range = RangeSize(bar->addr(), bar->size());
    DPRINTF(CxlMemory, "final range start=0x%lx, range size=0x%lx\n", range.start(), range.size());
    // range = AddrRange(0x100000000, 0x100000000 + 0x100000000); // 0x8000000=128MiB 0x100000000=4GiB
```

嵌套类Memory的访存函数

- `PciBar *bar = owner.BARs[0];`：owner是CxlMemory对象，该对象继承PciDevice类，拥有BARs等Pci设备的成员变量和成员函数
- `range = RangeSize(bar->addr(), bar->size());`：这里用系统配置完毕后写入bar空间的地址范围更新设备管理的地址范围，相当于告诉设备，你的存储空间在系统里面分配的地址是多少。但是这个range在后面有在`toHostAddr()`计算pkt实际读写地址用到，凡丰师兄之前写了这个range无法更新的问题，我就很丑陋的实现了一下。

```c
    if (pkt->cacheResponding()) {
        DPRINTF(CxlMemory, "Cache responding to %#llx: not responding\n", pkt->getAddr());
        return;
    }

    if (pkt->cmd == MemCmd::CleanEvict || pkt->cmd == MemCmd::WritebackClean) {
        DPRINTF(CxlMemory, "CleanEvict  on 0x%x: not responding\n", pkt->getAddr());
        return;
    }
```

这里貌似是cache相关的配置，也是直接return了，没有太理解

```c
    assert(pkt->getAddrRange().isSubset(range));

    uint8_t* host_addr = toHostAddr(pkt->getAddr());
    DPRINTF(CxlMemory, "host_addr = %p, pkt->getAddr = 0x%lx, pmemAddr = %p, range.start = 0x%lx\n", 
        *host_addr, pkt->getAddr(), *pmemAddr, range.start());
```

- assert这里用断言了一下pkt的地址确实是cxl设备管理范围的地址

- `uint8_t* host_addr = toHostAddr(pkt->getAddr());`：这个`toHostAddr()`在头文件里长这样：

  `inline uint8_t* toHostAddr(Addr addr) const { return pmemAddr + addr - range.start(); }`

  - `pmemAddr`是new得到的堆空间的首地址
  - `addr`是发往cxl设备的pkt的地址，肯定是`addr >= range.start()`
  - `range.start()`是系统给设备分配的地址范围的首地址
  - `addr - range.start()`就得到这个pkt实际想读写cxl内存的**偏移量**，偏移量再加pmemAddr这个首地址得到在这片堆空间中实际要读写的地址

```c
    if (pkt->cmd == MemCmd::SwapReq) {

        if (pkt->isAtomicOp()) {
            if (pmemAddr) {
                pkt->setData(host_addr);
                (*(pkt->getAtomicOp()))(host_addr);
            }
        } else {
            std::vector<uint8_t> overwrite_val(pkt->getSize());
            uint64_t condition_val64;
            uint32_t condition_val32;

            panic_if(!pmemAddr,
                     "Swap only works if there is real memory "
                     "(i.e. null=False)");

            bool overwrite_mem = true;
            // keep a copy of our possible write value, and copy what is at the
            // memory address into the packet
            pkt->writeData(&overwrite_val[0]);  // Write the data of the pkt to the vector
            pkt->setData(host_addr);            // Write the data of host_addr to pkt

            if (pkt->req->isCondSwap()) {
                if (pkt->getSize() == sizeof(uint64_t)) {
                    condition_val64 = pkt->req->getExtraData();
                    overwrite_mem = !std::memcmp(&condition_val64, host_addr, sizeof(uint64_t));
                } else if (pkt->getSize() == sizeof(uint32_t)) {
                    condition_val32 = (uint32_t)pkt->req->getExtraData();
                    overwrite_mem = !std::memcmp(&condition_val32, host_addr, sizeof(uint32_t));
                } else
                    panic("Invalid size for conditional read/write\n");
            }

            if (overwrite_mem)
                std::memcpy(host_addr, &overwrite_val[0], pkt->getSize());

            assert(!pkt->req->isInstFetch());
        }
```

这里的一堆if，是根据pkt中不同的命令来做不同的响应

这个SwapReq具体是什么情况下的指令我确实也没搞懂，目前应该还没有出现过，有可能没有用

```c
    } else if (pkt->isRead()) {
        assert(!pkt->isWrite());
        if (pmemAddr) {
            pkt->setData(host_addr);
            DPRINTF(CxlMemory, "%s read due to %s\n", __func__, pkt->print());
        }
    } else if (pkt->isInvalidate() || pkt->isClean()) {
        assert(!pkt->isWrite());
        // in a fastmem system invalidating and/or cleaning packets
        // can be seen due to cache maintenance requests

        // no need to do anything
    } else if (pkt->isWrite()) {
        if (pmemAddr) {
            pkt->writeData(host_addr);
            DPRINTF(CxlMemory, "%s write due to %s\n", __func__, pkt->print());
        }
        assert(!pkt->req->isInstFetch());
    } else {
        panic("Unexpected packet %s", pkt->print());
    }
```

这里的几个就比较好理解

- `pkt->isRead()`：就调用`pkt->setData(host_addr);`从host_addr取数据塞到pkt中
- `pkt->isWrite();`：就调用`pkt->writeData(host_addr);`将数据写入到host_addr中
- `pkt->isInvalidate() || pkt->isClean()`：也就是无效指令吧，啥也不做
- `assert(!pkt->req->isInstFetch());`：这个断言倒是头一回注意到，貌似是断言pkt中的请求不是取指

```c
    if (pkt->needsResponse()) {
        pkt->makeResponse();
    }
```

前面都处理完毕，最后给pkt设置命令为响应命令

> 其实这里我不是很懂，有个猜测就是：
>
> Atomic模式下，pkt是通过正常的访存路径发过来，就是cpu->membus->bridge->iobus->CXLMemory，但是完成了pkt的请求之后，如果pkt需要响应，比如pkt是读取命令，读到数据后，就直接把数据传给cpu了，再没有走访存路径，因为没看到相应的调用，只是把pkt的命令改成响应。
>
> Timing模式下，倒是有看到响应数据包的函数调用，因为是要对时序进行一个详细的建模。
>
> 有点迷惑

## src/mem/bridge.cc

bridge连接membus和iobus，预计在这里进行cxl协议的包格式转换，凡丰师兄以前是用到`src/mem/protocol/cxl_mem.hh`这个文件进行包格式转换，而且包括Timing、Atomic、Functional三种模式的修改，目前我这里就仅仅修改了Atomic的，即将包的命令转换成cxl.mem协议特有的`M2SReq`和`M2SRwD`

```c
    if (pkt->getAddr() >= 0x100000000 && pkt->getAddr() < 0x200000000) {
        DPRINTF(CxlMemory, "the cmd of pkt is %s, addrRange is %s.\n",
            pkt->cmd.toString(), pkt->getAddrRange().to_string());
        if (pkt->cmd == MemCmd::ReadReq) { pkt->cmd = MemCmd::M2SReq; }
        else if(pkt->cmd == MemCmd::WriteReq) { pkt->cmd = MemCmd::M2SRwD; }
        else { DPRINTF(CxlMemory, "the cmd of packet is %s, not a read or write.\n", pkt->cmd.toString()); }
        Tick cxl_delay = 2;
        Tick access_delay = memSidePort.sendAtomic(pkt);
        Tick total_delay = (delay + cxl_delay) * bridge.clockPeriod() + access_delay;
        DPRINTF(CxlMemory, "bridge delay=%ld, bridge.clockPeriod=%ld, access_delay=%ld, cxl_delay=%ld, total=%ld\n",
            delay, bridge.clockPeriod(), access_delay, cxl_delay, total_delay);
        return total_delay;
```

- pkt->getAddr() >= 0x100000000 && pkt->getAddr() < 0x200000000：固定了一个地址范围判断发向cxl设备的pkt，这里纯属迫不得已，一个简陋的实现，和师兄商量过，目前为了效果可以先这样写，之后如果要改进的话，就想办法让bridge知道它自己管理设备的地址范围，然后读取cxl设备的地址范围来判断。

  另外，BAR设置4GB以内可以用这个`(0x100000000，0x200000000)`范围，再大的话需要在这里做相应的修改

- 下面就是为bridge转换包格式加上了延迟2 tick（这个值的大小有待商榷吧，因为目前从测试结果来看看不出什么差别，如果有差别了可以再设置别的值）

## src/mem/packet.hh  packet.cc

新增了四个命令，分别如注释所示，注意.hh和.cc里面命令的**顺序要一致**，因为是enum类型，顺序变了，代表的类型就对不上了

- .hh里定义了pkt可以用的命令

- .cc里定义了命令的属性

```c
        // cxl.mem extended
        /** enum type:it is necessary to align the command item here 
         *  with the attribute item in the .cc file.
         */
        M2SReq, // Requset              (read)
        S2MDRS, // Data Response        (read resp)
        M2SRwD, // Requset with Data    (write)
        S2MNDR, // No Data Response     (write resp)
```

```c
    // for cxl.mem extended
    /* M2SReq */
    { {IsRead, IsRequest, NeedsResponse}, S2MDRS, "M2SReq"},
    /* S2MDRS */
    { {IsRead, IsResponse, HasData}, InvalidCmd, "S2MDRS" },
    /* M2SRwd */
    { {IsWrite, IsRequest, NeedsResponse, HasData}, S2MNDR, "M2SRwd"},
    /* S2MNDR */
    { {IsWrite, IsResponse}, InvalidCmd, "S2MNDR" }
```



## src/dev/x86/SouthBridge.py

这个py文件是将南桥部件连线用的，在南桥里将cxl设备连接上去。但是，fs文件貌似没有地方显示调用到SouthBridge.py，不过确实是连上去了

```python
    # CxlMemory
    cxlmemory = CxlMemory(pci_func=0, pci_dev=6, pci_bus=0)
```

将cxl设备实例化

```python
        self.ide.pio = bus.mem_side_ports
        self.cxlmemory.pio = bus.mem_side_ports
        if dma_ports.count(self.ide.dma) == 0:
            self.ide.dma = bus.cpu_side_ports
        if dma_ports.count(self.cxlmemory.dma) == 0:
            self.cxlmemory.dma = bus.cpu_side_ports
```

这里是模仿ide设备的连接方式，有一点不懂，还没来得及问师兄

- `cxlmemory.pio = bus.mem_side_ports`
- `cxlmemory.dma = bus.cpu_side_ports`

## configs/common/FSConfig.py

```python
# 475行
	x86_sys.bridge.ranges = [
        AddrRange(0xC0000000, 0xFFFF0000),      # (3GB,4GB-64kB)
        AddrRange(0x100000000, 0x300000000),    # (4GB,12GB)
        AddrRange(IO_address_space_base, interrupts_address_space_base - 1),
        AddrRange(pci_config_address_space_base, Addr.max),
    ]

```

这里是向bridge能够管理的地址范围内添加了一行`AddrRange(0x100000000, 0x300000000)`

因为cxl设备在bridge的下游，发向cxl设备的pkt的地址在`(0x100000000, 0x300000000)`范围内（姑且根据4gb的bar这样认为），如果bridge不知道这个地址范围的pkt在自己下游设备的话，它接收到就丢弃了，这样cxl设备就收不到pkt。

当添加了这一行让bridge知道自己下游还有设备地址范围在`(0x100000000, 0x300000000)`之中的，它就会向iobus转发pkt，这样cxl设备就能收到pkt了

## cxl.mem新的理解

### 传统的存储层次结构（访存过程）

传统的存储器层次结构就是，**每一层都是下一层的缓存**，内存DRAM是外存硬盘的cache。

CPU可直接访问的地址被称作全局地址空间，内存就被编入到这样的地址空间内，CPU可以直接使用load/store指令访问内存。但是外存或者说外设自带的存储空间并不会直接编入全局地址空间，主要是因为访问外存耗时太久以及过去CPU没有那么大的寻址范围。

CPU访问几乎每个外设的方式都是**通过读写外设上的寄存器**（其实也就是外设控制器controller）来进行的，通常包括控制寄存器、状态寄存器和数据寄存器三大类。**在系统启动的时候，会将每个外设控制器的寄存器编入全局地址空间，这样CPU就可以通过指令直接访问外设控制器的寄存器。**CPU通过指令告诉外设控制器，需要外设上的什么数据或者需要外设进行什么操作，由外设控制器去实际的操作外设的行为。

> 比如CPU要读取硬盘中的某个数据，也就是内存中未命中，CPU向硬盘控制器的端口（寄存器）发送以下命令：
>
> - 发送第一条指令，指明是硬盘读命令，以及其他的参数，比如读取完成需不需要发送中断
> - 发送第二条指令，指明应该读的逻辑块号
> - 发送第三条指令，指明读到的硬盘数据应该存储在内存的什么地址
>
> 随后就会进行DMA操作，硬盘控制器读取到端口的命令，去进行读数据，然后将数据复制到内存的相应位置。完毕后DMA向CPU发一个中断，表示读取已完成，CPU就会去内存中取数据



根据CPU体系结构的不同，对全局地址空间的编址方式也不同，根据对外设寄存器的访问方式不同，分为**统一编址**和**独立编址**：

- 统一编址（ARM、PowerPC）：**将外设寄存器看作内存的一部分，寄存器参与内存统一编址**，访问寄存器就通过访问一般的内存指令进行，所以这种CPU没有专门用于设备I/O的指令。这就是所谓的“I/O内存”方式。
- 独立编址（X86）：**将外设寄存器看成一个独立的IO地址空间**，所以访问内存的指令不能用来访问这些寄存器，而要为对外设寄存器的读写设置专用指令，如IN和OUT指令。这就是所谓的” I/O端口”方式 。32位X86有64K的IO空间

![](https://img2020.cnblogs.com/blog/908780/202004/908780-20200422140016269-1343778564.png)

现代计算机中外设与CPU的连接方式几乎都是PCIe总线了，对于硬盘，目前还存在SATA接口的，不过性能也没有PCIe好，正在慢慢被取代。所有的这些外设，其实与PCIe总线直接连接的都是外设控制器（含有一些寄存器），所以对于硬盘而言，准确的说硬盘控制器才是PCI设备，而后面的硬盘只是存储介质。这一点如果在linux下使用`lspci -v`指令，就可以看到都是controller：

- 显卡：VGA compatible controller
- 网卡：Network controller
- SATA接口的硬盘：SATA controller
- PCIe接口的硬盘：Non-Volatile memory controller

以PCIe总线为例，在系统枚举和配置PCI设备时，会依次为每个设备声明的BAR空间容量分配全局地址，**这些BAR空间声明的就是外设寄存器，而不是外存的容量**，CPU后续可以直接访问这些外设寄存器，间接从外存中读写数据。

> /dev/hda一般是指IDE接口的硬盘，/dev/hda指第一块硬盘，/dev/hdb指第二块硬盘,等等；
>
> /dev/sda一般是指SATA接口的硬盘，/dev/sda指第一块硬盘，/dev/sdb指第二块硬盘，等等。
>
> /dev/nvme一般是指peci接口的硬盘，/dev/nvmen0指第一块硬盘，/dev/nvmen1指第二块硬盘，等等。

PCI协议中规定了三种空间：

- 配置空间：PCI设备和PCI桥内部的空间，用于系统枚举和配置设备
- Memory空间：可以理解为全局地址空间，Memory空间包含内存和外设IO空间映射（MMIO）等。空间大小只受寻址范围的限制
- IO空间：访问外设寄存器的空间，其实就是上述独立编址中的IO地址空间。过去的IO设备几乎都只能通过IO空间访问，但是这种方式局限很大。32位X86有64K的IO空间

在配置BAR空间的时候，可以指定将BAR空间映射到IO空间还是Memory空间，对于统一编址的体系结构而言，可以直接将其映射到Memory空间，但是对于x86而言，外设寄存器还只能映射到IO空间，后来出现了MMIO机制，可以让外设寄存器映射到Memory空间（全局地址空间），这样CPU就可以和读写内存一样的指令访问外设寄存器，而不用单独的IN和OUT指令，大小也没有了限制。

针对IO空间和Memory空间有两种CPU和外设通信的方式（也就是驱动和设备通信）：

- PIO（IO空间）：也被称为**IO端口访问**。就是使用单独的IO指令：OUT/IN，OUT用于write操作，in用于read操作
- MMIO（Memory空间）：也被称为**IO内存访问**。这是将寄存器的地址空间直接映射到全局地址空间，全局地址空间往往会保留一段内存区用于这种MMIO的映射，这样系统可以直接使用普通的访存指令直接访问设备的寄存器。（目前最常用的方式）这样做需要在中间加一个映射的操作，因为在系统中软件都不能直接访问物理地址，不管是内核态还是用户态，它们能看到的都是虚拟地址。也就是驱动程序不能直接去读写外设寄存器，需要ioremap()将寄存器物理地址映射到内核的虚拟地址空间，然后通过访存指令访问虚拟地址，这样来访问外设寄存器。

> 这么多繁杂的概念、兼容的设计，应该都是为了x86的独立编址服务

参考

[PCI的IO空间和memory空间](https://blog.csdn.net/fengxiaocheng/article/details/103258791)

[MMIO和PIO](https://www.cnblogs.com/beixiaobei/p/10608356.html)

[IO空间，IO端口，MMIO](https://www.cnblogs.com/yi-mu-xi/p/10939735.html)

[PCIe扫盲——Memory & IO 地址空间](http://blog.chinaaet.com/justlxy/p/5100053319)

[操作系统是如何访问IO设备的？](https://zhuanlan.zhihu.com/p/376675657)

### cxl.mem定义下的存储层次结构

首先，cxl.mem或者说cxl type3设备是在PCI的基础上建立的，cxl扩展内存设备首先是一个pci设备，那么上文中cpu如何访问传统的pci硬盘设备呢？通过访问硬盘控制器的寄存器来实现读写硬盘，也就是说，CPU只能访问到硬盘控制器的寄存器，告诉控制器它需要硬盘的什么数据，存放到内存的什么位置，然后DMA去接管下面的工作，数据在内存中放好后，CPU去内存中取。

这样越来越高带宽低延迟的PCIe总线应该是得不到充分利用的，而且目前的64位CPU寻址范围非常大，不如**直接将一个“外存”的全部容量都并入全局地址空间，让CPU可以直接像访问内存那样使用load/store指令去访问这一大片“外存”。**也就免去了经DMA这一手，将数据先复制到内存，再从内存中读数据，可以直接将冷数据移动到cxl扩展内存中，使用的时候CPU去直接访问即可。

>  当然这里的外存具体的存储介质不一定是固态硬盘的存储介质，可以使用更快速的介质。这样可以充分利用PCIe6.0近乎内存的高速带宽，让CPU获得了一片和内存差不多的扩展内存去使用。当然这片扩展内存在cxl.mem协议下貌似还不能被CPU缓存，不过在内存捉紧的局面下，能不能被cache倒也无伤大雅了。

那么这样cxl.mem就重新定义了一部分存储层次结构，也就是在内存旁边又加了一片扩展内存，只不过是使用的PCIe接口。仍然是有下一层外存，内存仍然是外存的cache。
