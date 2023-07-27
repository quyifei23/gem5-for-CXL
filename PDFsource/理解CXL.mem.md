## 理解CXL.mem

> 参考：
>
> [CXL 学习笔记](https://jia.je/hardware/2022/11/20/cxl-notes/)
>
> [CxlMemory设备代码解读](https://github.com/quyifei23/gem5-for-CXL/blob/master/PDFsource/%E3%80%90CXL%E3%80%91CxlMemory%E8%AE%BE%E5%A4%87%E4%BB%A3%E7%A0%81%E8%A7%A3%E8%AF%BB.md)

### cxl.mem与传统存储层次的区别

传统存储层次都是上一层存储设备作为下一层存储设备的缓存，对于PCI设备，CPU对其存储区域的访问只能通过将其中的数据复制到内存的相应地方才能访问，这就导致效率低下，加入了数据复制的开销，

CPU可以直接访问的地址称为全局地址空间，内存就被编入这个空间中。但是外存或者外部设备自带的存储空间并不会直接编入全局地址空间，主要原因是访问这些存储空间时间久（现在PCIe提供高带宽低延迟的传输能力），而且之前的CPU没有这么大的寻址能力（现在CPU有64为寻址空间，超大寻址能力）因此需要一种新的体系或协议来加速外设的访问。cxl.mem就是为了解决这样的问题。

cxl.mem或者说cxl type3设备是在PCI的基础上建立的，cxl扩展内存设备首先是一个pci设备。之前CPU只能访问到硬盘控制器的寄存器，告诉控制器它需要硬盘的什么数据，存放到内存的什么位置，然后DMA去接管下面的工作，数据在内存中放好后，CPU去内存中取。

这样越来越高带宽低延迟的PCIe总线应该是得不到充分利用的，而且目前的64位CPU寻址范围非常大，不如**直接将一个“外存”的全部容量都并入全局地址空间，让CPU可以直接像访问内存那样使用load/store指令去访问这一大片“外存”。**也就免去了经DMA这一手，将数据先复制到内存，再从内存中读数据，可以直接将冷数据移动到cxl扩展内存中，使用的时候CPU去直接访问即可。

> 当然这里的外存具体的存储介质不一定是固态硬盘的存储介质，可以使用更快速的介质。这样可以充分利用PCIe6.0近乎内存的高速带宽，让CPU获得了一片和内存差不多的扩展内存去使用。当然这片扩展内存在cxl.mem协议下貌似还不能被CPU缓存，不过在内存捉紧的局面下，能不能被cache倒也无伤大雅了。

那么这样cxl.mem就重新定义了一部分存储层次结构，也就是在内存旁边又加了一片扩展内存，只不过是使用的PCIe接口。仍然是有下一层外存，内存仍然是外存的cache。



### CXL.mem的简单介绍

CXL.mem用于扩展内存，根据类型不同，它可能单独使用，也可能和CXL.cache配合使用。对于Type3设备，使用的是HDM-H(Host-only Coherent)一致性模型。



对于CXL.mem，两端的设备是Master和Subordinate。

从Master到Subordinate的消息(M2S)有三类：

+ Request(Req)
+ Request with Data(RwD)
+ Back-Invalidation Response(BIRsp)

从Subordinate到Master的消息(S2M)有三类：

+ Response without Data(NDR,No Data Response)
+ Response with Data(DRS,Data Response)
+ Back-Invalidtion Snoop(BISnp)

其中比较特别的是Back-Invalidation，这个的目的是让Device可以通过Snoop修改Host中缓存了Device内存中的数据的缓存行。

对于Type3的设备(无CXL.cache)来说，Device就是一个扩展的内存，比较简单只需要支持读写内存就可以了。Host发送MemRd，Device相应MemData；Host发送MemWr，Device相应Cmp。



### Introduction

CXL.mem是介于CPU和存储空间事物层的接口。协议可以用于根据存储空间位置的不同而导致的不同类型的存储，比如位于CPU中的存储控制器，位于加速设备中的存储控制器。

The view of the address region must be consistent on the CXL.mem path between the Host and the Device .

无论在系统中的哪一条执行CXL.mem协议的Host-Device路径上，地址空间的视图都是一致的。我的理解是CXL.mem会维护一个包含相连Device内存存储空间全局地址空间。



### CXL.mem Channel Description

CXL.mem信息传输的通道是相互独立工作的，为了维护正在发送的信息。CXL.mem定义了六种通道，分别对应着上述的六种信息类型。


