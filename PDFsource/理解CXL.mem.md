## 理解CXL.mem

https://jia.je/hardware/2022/11/20/cxl-notes/

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



### Qos Telemetry