## 对PCI设备的理解

> [Chapter 6 PCI](https://tldp.org/LDP/tlk/dd/pci.html)

### PCI Address Spaces

CPU和PCI设备需要一段共享的内存空间，这段空间用于控制PCI设备以及设备之间的信息传递，主要包括控制信息和状态寄存器。这些寄存器用于控制PCI设备以及读取他们的状态。但是如果这段共享的内存空间是位于系统内存上的，会导致每当PCI设备读取内存的时候都会导致CPU无法读取内存，严重降低了CPU的效率，同时若PCI设备能随意读取内存会使得一个外部连接的设备就可以轻易改变内存中的值，这是相当不安全的。所以PCI设备拥有它们自己的内存空间。CPU可以读取到这些内存空间，但是PCI设备只能通过DMA通道来与系统内存通信。PCI有三类内存空间：

+ PCI I/O
+ PCI Memory
+ PCI Configure space

CPU可以访问所有的PCI设备内存空间，PCI Configure space用于存储PCI的配置信息，从而在初始化的时候被利用。



### PCI Configuration Headers

包括PCI-PCI Bridge在内的所有PCI设备都含有PCI Configuration Header



**Device Identification**

A unique number describing the device itself. For example, Digital's 21141 fast ethernet device has a device identification of *0x0009*.

**Vendor Identification**

A unique number describing the originator of the PCI device. Digital's PCI Vendor Identification is *0x1011* and Intel's is *0x8086*.

**Status**

This field gives the status of the device with the meaning of the bits of this field set by the standard. .

**Command**

By writing to this field the system controls the device, for example allowing the device to access PCI I/O memory.

**Class Code**

This identifies the type of device that this is. There are standard classes for every sort of device; video, SCSI and so on. The class code for SCSI is *0x0100*.

**Base Address Registers**

These registers are used to determine and allocate the type, amount and location of PCI I/O and PCI memory space that the device can use.

**Interrupt Pin**

Four of the physical pins on the PCI card carry interrupts from the card to the PCI bus. The standard labels these as A, B, C and D. The *Interrupt Pin* field describes which of these pins this PCI device uses. Generally it is hardwired for a pariticular device. That is, every time the system boots, the device uses the same interrupt pin. This information allows the interrupt handling subsystem to manage interrupts from this device,

**Interrupt Line**

The *Interrupt Line* field of the device's PCI Configuration header is used to pass an interrupt handle between the PCI initialisation code, the device's driver and Linux's interrupt handling subsystem. The number written there is meaningless to the the device driver but it allows the interrupt handler to correctly route an interrupt from the PCI device to the correct device driver's interrupt handling code within the Linux operating system. See Chapter [ interrupt-chapter](https://tldp.org/LDP/tlk/dd/interrupts.html) on page for details on how Linux handles interrupts.



### PCI I/O and PCI Memory Address

PCI设备可以通过这两段空间来和CPU上的驱动设备进行交流。在PCI设备被CPU识别并设置好之前，这两段空间是不能被访问的。同时CPU上的PCI驱动设备只能访问PCI I/O和PCI Memory Address，而不能访问PCI Configuration Address，只能通过PCI Configuration code来读写PCI Configuration Address。



### PCI Bridge

PCI Bridge也是一种PCI设备，用于连接系统的PCI总线。通过PCI总线的连接，Linux系统可以支持更多的PCI设备。