## 对PCI和PCIe的理解

> [深入PCI与PCIe之一：硬件篇](https://zhuanlan.zhihu.com/p/26172972)
>
> [PCIe基础篇——Switch/Bridge/Root Complex/EndPoint](https://blog.csdn.net/u013253075/article/details/119045277)



### Root Complex

简称RC，CPU和PCle总线之间的接口，可能包含几个组件(处理器接口、DRAM接口等)，甚至可能包含几个芯片。RC位于PCI倒立树拓扑的“根”，并代表CPU与系统的其余部分进行通信。但是，规范并没有仔细定义它，而是给出了必需和可选功能的列表。从广义上讲，RC可以理解为系统CPU和PCle拓扑之间的接口，PCle端口在配置空间中被标记为“根端口”。



### Bridge

桥提供了与其他总线(如PCI或PCI- x，甚至是另一个PCle总线)的接口。如图中显示的桥接有时被称为“转发桥接”，它允许旧的PCI或PCIX卡插入新系统。相反的类型或“反向桥接”允许一个新的PCle卡插入一个旧的PCI系统。



### Switch

提供扩展或聚合能力，并允许更多的设备连接到一个PCle端口。它们充当包路由器，根据地址或其他路由信息识别给定包需要走哪条路径。是一种PCIe转PCIe的桥。



### Endpoint

处于PCIe总线系统拓扑结构中的最末端，一般作为总线操作的发起者（initiator，类似于PCI总线中的主机）或者终结者（Completers，类似于PCI总线中的从机）。显然，Endpoint只能接受来自上级拓扑的数据包或者向上级拓扑发送数据包。细分Endpoint类型的话，分为Lagacy PCIe Endpoint和Native PCIe Endpoint，Lagacy PCIe Endpoint是指那些原本准备设计为PCI-X总线接口的设备，但是却被改为PCIe接口的设备。而Native PCIe Endpoint则是标准的PCIe设备。其中，Lagacy PCIe Endpoint可以使用一些在Native PCIe Endpoint禁止使用的操作，如IO Space和Locked Request等。Native PCIe Endpoint则全部通过Memory Map来进行操作，因此，Native PCIe Endpoint也被称为Memory Mapped Devices（MMIO Devices）
