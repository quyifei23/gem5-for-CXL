/*
 * Copyright (c) Krishna Srinivasan
 * All rights reserved
 *
 * The license below extends only to copyright in the software and shall
 * not be construed as granting a license to any other intellectual
 * property including but not limited to intellectual property relating
 * to a hardware implementation of the functionality of the software
 * licensed hereunder.  You may use the software subject to the license
 * terms below provided that you ensure that this notice is replicated
 * unmodified and in its entirety in all distributions of the software,
 * modified or unmodified, in source code or in binary form.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Authors: Krishna Parasuram Srinivasan
 */

/**
 * @file
 * Declarations of classes in a PCI Express Link
 */


#ifndef __DEV_PCIE_LINK_HH__
#define __DEV_PCIE_LINK_HH__

#include <deque>
#include<queue>
#include <string>
#include "base/types.hh"
#include "mem/port.hh"
#include "mem/packet.hh"
#include "params/PCIELink.hh"
#include "debug/PCIELink.hh"
#include "sim/eventq.hh"
#include "sim/sim_object.hh"

/**
 * TLP request and response header size is 12B with 32 bit PCI Express addresses
 */
#define PCIE_REQUEST_HEADER_SIZE 12  

#define PCIE_RESPONSE_HEADER_SIZE 12

/**
 * Data Link Layer overhead added to a TLP : Sequence number + LCRC - 6B
 */
#define DLL_OVERHEAD 6

/**
 * Physical layer overhead is 2B, due to framing characterss
 */
#define PHYSICAL_OVERHEAD 2

/**
 * 8b/10b encoding used for Gen 2 PCI Express
 */
#define ENCODING_FACTOR 1.25     

/**
 * Size of DLLP is 6B
 */
#define DLLP_SIZE 6 


/**
 * This header file contains class definitions used in a PCI Express Link.
 * There are 4 main classes declared in this file
 * 
 * 1. DataLinkPacket - Used to represent a data link layer packet in Gem5.
 *                     Data Link Layer packets are generated and consumed at 
 *                     the ends of the PCI Express Link.
 * 
 * 2. PCIELinkPacket - This class is used to represent a packet transmitted 
 *                     across the link. The transmitted packet can be either a 
 *                     Gem5 packet or a DataLinkPacket(DLLP).
 *
 * 3. ReplayBuffer   - This class represents the replay buffer used to implement
 *                     reliable transmission across a PCI Express Link. 
 *                     Each link interface has a replay buffer.
 *
 * 4. PCIELink       - This class represents the actual PCI Express Link in Gem5 
 *                     It consists of 2 link interfaces, with each link 
 *                     interface used to transmit packets from devices attached  
 *                     on that end of the link. 
 * 
 * It is important to note that in this PCI Express Link implementation, Gem5
 * packets are considered to represent the Transaction Layer Packets (TLPs)
 * present in the PCI Express protocol. The size of the Transaction Layer packet
 * is based on the size of the Gem5 packet, as well as overheads present in the
 * PCI Express protocol, as defined above.
 * Both TLPs and DLLPs are encapsulated in a PCIELinkPacket before being sent
 * out onto the unidirectional link attached to a particular link interface.
 */


/**
 * Class used to represent an Ack DLLP. The Ack DLLP indicates that a packet
 * has been successfully received by the device on one end of the link. 
 * The Ack DLLP contains a sequence number that indicates the sequence 
 * number of the successfully received packet
 */

namespace gem5
{
    class DataLinkPacket
    {
    public:
        /**
         * This variable stores the sequence number associated with Ack DLLP
         */ 
        uint64_t seqNum ; 
        
        /**
         * crc contained in Ack DLLP. Not used. 
         */
        uint16_t crc ; 
        
        /**
         * Function to return the size of the DLLP, which is a constant value
         * since a DLLP does not contain a payload.
         *
         * @return The size in Bytes of the DLLP
         */
        int 
        getSize() 
        { 
        return DLLP_SIZE ; 
        }  
        
        /**
         * Constructor to create a new data link packet (Ack)
         * 
         * @param _seq_num The sequence number of the packet acknowledged
                            by this Ack
        */
    
        DataLinkPacket(int seq_num) : seqNum(seq_num)
        {}
    } ;  


    /**
     * Class used to represent a PCI Express packet transmitted across a link.
     * This packet can either be a Gem5 packet (TLP) or a DLLP. If the packet is a
     * Gem5 packet, the pkt pointer is valid. If the packet is a DLLP, the dllp 
     * pointer is valid
     */
        
    class PCIELinkPacket
    {
    public:
    
        /**
         * Pointer to a Gem5 packet if the PCIELinkPacket encapsulates a Gem5 pkt
         */ 
        PacketPtr pkt ;
        
        /**
         * Pointer to a DataLinkPacket, if this PCIELinkPacket encapsulates an Ack
         */ 
        DataLinkPacket * dllp ;

        /**
         * Does the PCIELinkPAcket encapsulate a TLP ? 
         */
        bool isTLP ;
        
        /**
         * Does the PCIELinkPacket encapsulate a DLLP ? 
         */
        bool isDLLP ;

        /**
         * Sequence number field of TLP if TLP is encapsulated
         */
        uint64_t seqNum ; 
    
        /**
         * Constructor for the PCIE Link Packet. 
         * Assigns the pkt and dllp pointers as NULL
         * Code that creates a new PCIELinkPAcket has to assign variables based on
         * whether a TLP or DLLP is being created
         */
        PCIELinkPacket() ;

        /**
         * Returns the size of the PCIELinkPacket, depending on whether a TLP or 
         * DLLP is encapsulated within. Overheads such as header size and sequence 
         * numbers and framing characters are taken into account here. 
         *
         * @ret The effective size of the PCIELinkPacket (not the size of the class)
         */  
        int getSize() ;                     
        
    } ; 


    /**
     * Class representing a replay buffer, which is implemented as a queue.
     * Each link interface contains a replay buffer. Replay Buffers store 
     * transmitted TLPs (Gem5 packets) till an Ack is received corresponding to 
     * the sequence number of the TLP. Once the Ack is received, the stored TLPs
     * are deleted and the buffer can accept more packets
     */
    class ReplayBuffer
    {
    public:

        /**
         * Size of replay buffer in packets
         */  
        int maximumSize ;
        
        /**
         * Double ended queue used to implement the replay buffer
         */
        std::deque<PCIELinkPacket*> queue ;
        
        /**
         * Constructor for the ReplayBuffer. Push as many NULL ptrs into the dequeue
         * as there are entries in the replay buffer. A dequeue entry will 
         * contain a non NULL value if that pointer points to a packet stored in the
         * buffer.
         *
         * @param Size of replay buffer
         */
        ReplayBuffer(int max_size) ; 

        /**
         * Calculates the present number of entries in the replay buffer. There is 
         * a pointer with a non-null value for each entry in the replay buffer. The
         * replay buffer is formatted such that the NULL entries are toward the back
         *
         * @return size of the replay buffer. 
         */ 
        int size() ; 
    
        
        /**
         * Push a ptr to a newly transmited PCIELinkPacket to the back of the buffer
         * Replace the first NULL ptr with a ptr to the transmitted PCIELinkPacket
         * 
         * @param ptr to the PCIELinkPacket to store in buffer.
         */ 
        void pushBack (PCIELinkPacket* ptr) ;         
    
        /**
         * Remove the first entry in the replay buffer and return a ptr to the 
         * PCIELinkPacket. Also push back a NULL pointer to the dequeue, so that the
         * size of the dequeue remains constant.
         * @return Return the PCIELinkPacket ptr removed from the buffer
         */
        PCIELinkPacket * popFront() ;

        /**
         * @return return the first element of the buffer
         */
        PCIELinkPacket * 
        front()                     
        {
            return queue.front() ; 
        }

        /**
         * Get the nth element stored in the replay buffer
         * @param idx of the element to return
         * 
         * @return The nth element stored in the buffer
         */
        PCIELinkPacket * get(int idx) ; 
        
        
        /**
         * Is the replay buffer empty ? Check if there is a non null value stored
         *
         * @return True if buffer is empty, false otherwise
         */
        bool empty() ;                                 
    
    };
            

    /**
     * This class represents a PCIE Link in Gem5. The PCIE link can be visualized 
     * as containing 2 interfaces. Each interface is attached through a master/slave
     * port pair to the device on that end of the link. Each interface is attached 
     * to a unidirectional link(based on Ethernet Link),that can be used to transmit 
     * packets to the other interface. An interface's transmit function is called to 
     * send a packet to the other interface,while the interface's receive function  
     * is called to receive a packet from the unidirectional link attached to the 
     * other interface. Each link interface also contains a replay buffer, to hold 
     * unAcked transmitted Gem5 packets
     */  
    class PCIELink : public SimObject
    {
    public :
        // Forward Declarations
        class LinkRequestPort ;
        class LinkResponsePort ; 

        /**
         * The Link Class used to represent a link interface + unidirectional link 
         * that the interface transmits packets on.
         *
         * The unidirectional link is based on the ethernet link in Gem5. It 
         * represents a unidirectional PCIE link, and can be configured with the 
         * number  of lanes and the bandwidth and delay(assumed to be 0). The  
         * unidirectional link accepts packets from the link interface via the 
         * transmit() function. The unidirectional link schedules an event for 
         * a future time based on the size of the PCIELinkPacket and the bandwidth 
         * and number of lanes of the link. Once the event occurs, the packet is
         * transmitted to the other interface belonging to the PCIELink.  
         *
         * Each link interface accepts Gem5 packets from its master and slave 
         * ports, denoted as masterPortSrc and slavePortSrc respectively. These 
         * attached master and slave ports are to be connected to a PCIE device's
         * slave and master ports respectively. A PCIE device in this context refers
         * to either a switch/root port or an endpoint(e.g. NVMe, NIC, etc.)
         *
         * In addition to receiving Gem5 packets from a device, each link interface
         * also receives PCIELinkPackets from the other interface of the PCIELink,
         * via the other interface's assigned unidirectional link. Each
         * PCIELinkPacket can either encapsulate a Gem5 packet or a DataLinkPacket. 
         * If the received PCIELinkPacket encapsulates a Gem5 packet(TLP), then the
         * Gem5 packet is sent to either the attached master or slave port based on 
         * whether the packet is a request or a response. 
         * 
         * DataLinkPackets originate from and are consumed by link interfaces. Each
         * DataLinkPacket is an acknowledgement message for a particular TLP(Gem5 
         * packet) that is successfully received by a link interface. Successfully
         * received in this context indicates that devices attached to the interface
         * accept the Gem5 packet. 
         *
         * Each interface implements a simplified version of the Ack/NAK protocol in
         * the data link layer of real PCIE devices. Since unidirectional links are 
         * assumed to be error free, only Ack DLLPs are used. When an interface 
         * transmits a TLP (Gem5 packet), a pointer to the transmited PCIELinkPacket
         * is stored in its replay buffer. If the other interface receives the TLP
         * successfully, it sends back an Ack to acknowledge this TLP. The sending 
         * interface, on receipt of this Ack removes the replay buffer entry 
         * corresponding to the PCIELinkPAcket representing the acknowledged TLP.
         *
         * Each interface also maintains a replay timer. If after the timeout 
         * interval, an Ack is not received for a transmitted TLP, all stored
         * PCIELinkPackets are retransmitted from the replay buffer. Conversely,
         * the timer is reset on receipt of an ACk DLLP. 
         *
         * An Ack DLLP need not be sent for every received TLP. An ack timer is 
         * maintained to send an Ack back for all successfully received TLPs on 
         * timer expiration.
         * 
         * For more info on Ack/NAK protocol, refer to PCI Express System 
         * Architecture book. 
         */

        class Link 
        {
        public : 
            const std::string __name ; 
        
        /**
            * Ptr to parent simobject, used to schedule events in Gem5
            */
            PCIELink * parent ;           
            
            /**
             * Number of ticks a byte takes to be transmitted on the link
             */
            const double ticksPerByte;
                    
            /**
             * Propagation delay across link. Set to 0. 
             */
            const Tick linkDelay;              
            
            /**
             * Not used. Set to 0.
             */
            const Tick delayVar; 

            /**
             * Pointer to the master port attached to the link interface. Is 
             * connected to the slave/PIO port of a Gem5 device or root/switch port
             */
            LinkRequestPort * masterPortSrc ;
            
            /**
             * Pointer to the slave port attached to the link interface. Is 
             * connected to the master/DMA port of a Gem5 device or root/switch port
             */
            LinkResponsePort * slavePortSrc ;
            
            /**
             * Pointer to the master port attached to the other interface of the 
             * PCIELink.
             */                                 
            LinkRequestPort * masterPortDest ;
            
            /**
             * Pointer to the slave port attached to the other interface of the
             * PCIELink.
             */
            LinkResponsePort * slavePortDest ;
        
            /**
             * Number of lanes making up each unidirectional link. The bandwidth
             * scales proportionately with number of lanes present. The number of
             * lanes making up a link can be 1,2,4,8,12,16 or 32. 
             */
            int lanes ; 
            
            /**
             * Packet currently being transmitted on this unidirectional link
             */
            PCIELinkPacket * packet ;    
            
            /**
             * Indicates whether a packet is currently being transmitted on the link
             * @return true if a packet is transmitted on the unidirectional link.
             *         false if link is free
             */
            bool
            busy() 
            { 
                return packet != NULL ; 
            } 
        
            /**
             * Queue of inflight packets on the unidirectional link
             */
            std::deque<std::pair<Tick , PCIELinkPacket*>> txQueue ;
        
            /**
             * Denotes the function called when a PCIELinkPacket is finished
             * being transmitted across the unidirectional link.
             */
            EventFunctionWrapper doneEvent ;
            
            /**
             * Denotes the function called when a PCIELinkPacket is ready to be sent
             * to the receiving interface
             */
            EventFunctionWrapper txQueueEvent ;
            
            /**
             * Denotes function called when a timeout occurs and retransmission of 
             * packets on the unidirectional link needs to take place. 
             */ 
            EventFunctionWrapper timeoutEvent ;
            
            /**
             * Denotes function called when an Ack needs to be sent back to the 
             * other interface to indicate the successful reception of TLPs (Gem5 
             * packets)
             */
            EventFunctionWrapper ackEvent ; 
            
            /**
             * Replay buffer present in the link interface to hold transmitted but
             * unAcked PCIELinkPackets that encapsulate a TLP. Remember that only
             * TLPs(Gem5 packets) need an Ack, not DLLPs. 
             */ 
            ReplayBuffer buffer ; 
            
            /**
             * Link destructor. Needs to be implemented
             */
            ~Link(){}

            /**
             * Function used to transmit PCIELinkPackets on the unidirectional link.
             * The DoneEvent is scheduled after a duration corresponding to the 
             * time taken to transmit a PCIELinkPacket on the link, which depends
             * on ticksPerByte, number of lanes present in the unidirectional link,
             * etc. If a packet is currently being transmitted, return false.
             *
             * @param pkt Pointer to PCIELinkPAcket to transmit on unidirect. link.
             * @return successful transmission or not
             */    
            bool transmit(PCIELinkPacket * pkt) ;
            
            /**
             * Function called by an interface's attached master and slave ports to
             * transmit a Gem5 packet on the unidirectional link. Each Gem5 packet 
             * is encapsulated within a PCIELinkPacket before being transmitted out
             * onto the link. The PCIELinkPackets encapsulating a Gem5 packet are 
             * assigned a sequence number by the sending interface and are stored
             * in the sending interface's replay buffer after transmission.
             * @param pkt Gem5 packet to be transmitted. 
             * @param master Is the master port attached to interface sending this
             * packet ? 
             * @return Packet successfully transmitted or needs to be resent ?.
             */
            bool linkTransmitInterface(PacketPtr pkt, bool master) ;       

            /**
             * Function called by a link to send a packet to a receiving interface. 
             * Upon reception of a PCIELinkPAcket encapsulating a TLP, a sequence 
             * number check is performed to avoid out of order packets. The Gem5 
             * packet is extracted from the PCIELinkPacket, is sent to the attached
             * slave or master port. If the device attached to this interface 
             * accepts the Gem5 packet (TLP), an Ack is scheduled to be sent back
             * to the sending interface with the sequence number of the received 
             * PCIELinkPacket.
             *
             * Upon reception of a PCIELinkPacket encapsulating a Ack DLLP, entries
             * in the replay buffer are freed based on the sequence number of the 
             * Ack. All PCIELinkPAckets with a seq. number < Ack seq. number are 
             * freed from the replay buffer as reliable in order transmission of
             * these packets is assumed to have taken place
             *
             * @param packet PCIELinkPAcket received from the unidirectional link. 
             */
            void linkReceiveInterface(PCIELinkPacket * packet )  ;          
    
            /**
             * Sequence number to be assigned to TLP before transmission across the
             * link. 
             */
            uint64_t sendSeqNum ;                                          

            /**
             * Sequence number of the next TLP to be received from the other 
             * interface making up the PCIELink. 
             */
            uint64_t recvSeqNum ;                                            

            /**
             * Sequence number of the last packet an Ack was sent for. Used to 
             * avoid sending duplicate Acks. 
             */
            uint64_t lastAcked ; 
            
            /**
             * Size of the replay buffer present in the link interface
             */
            int  maxQueueSize ;                                            

            /**
             * Time after which the retry timer expires and the contents of the 
             * replay buffer are replayed. 
             *
             * Consider the following calculations.  
             * A=(Maximum payload size * Time to transmit a byte on the link)/Lanes
             * B = AckFactor - 1.4 for x1, 1.4 for x2. 1.4 for x4, 2.5 for x8 and 
             *     with 128B Maximum Payload Size
             * C = Internal Delay
             * D = Rx_L0_Adjustment
             *
             
            * The retryTime is calculated as (A*B + C)*3 + D. 
            * In the implemented PCIELink, both the internal delay and 
            * Rx_L0_Adjustment times are taken to be 0. 
            *
            * Maximum PAyload Size = Gem5 Cache Line Size
            * Time to transmit a byte on the link = ticksPerByte
            * Lanes = Number of lanes configured in each unidirectional link. 
            */   
            uint32_t retryTime ;
            
            /**
             * Was there a Gem5 response packet that could not be sent earlier due
             * to the link being occupied ? Need to resend it once the link frees
             * up ? 
             */                                    
            bool retryResp ;
    
            /**
             * Was there a Gem5 request packet that could not be sent earlier and 
             * needs to be resent ? 
             */
            bool retryReq ;
    
            /**
             * TLP which is currently being transmitted on the unidirectional link.
             * Could also represent the next TLP to be transmitted on the 
             * unidirectional link if a DLLP or retransmission was already being 
             * transmitted on the link when the TLP Gem5 packet was received from
             * the device attached to the interface.  
             */
            PCIELinkPacket * replayPacket ;
    
            /**
             * Next DLLP to be sent out onto the link once the current PCIELinkPAck.
             * is finished being transmitted on the unidirectional link
             */
            PCIELinkPacket * replayDLLP ;
    
            /**
             * Indicates whether the contents of the replay buffer in the interface
             * need to be replayed.
             */
            bool retransmit ;
            
            /**
             * Indicates the replay buffer entry to be retransmitted next
             */ 
            int retransmitIdx ;
            
            /**
             * Double Pointer to the other Link making up the PCIELink
             */
            Link ** otherLink ;  
            
            /**
             * Function called as soon as a PCIELinkPAcket is finished being 
             * transmitted on the unidirectional link. This function calls the 
             * linkReceiveInterface() function of the receiving interface. 
             */
            void processTxQueue() ;
    
            /**
             * A PCIELinkPacket is done being transmitted on the unidirectional link
             * This function does the following steps.
             * 1. Set the packet variable to false to indicate that the unidirection
             *    link is free.
             * 
             * 2. If the packet which just tranmsitted encapsulates a TLP, then 
             *    check if retryReq or retryResp is true. If one of them is set, 
             *    then make the master or slave ports of the interface ask for a
             *    packet retransmission from the device attached to this interface. 
             *
             * 3. Now choose the next packet to be transmitted on the link, based on
             *    the following priority. Pending ACK DLLP (if any) > Next TLP to be
             *    retransmitted (if any) > Next TLP to be transmitted for the first
             *    time(if any). 
             */
            void txDone() ;
            
            /**
             * Once a PCIELinkPAcket is finished being transmitted on the unidirect.
             * link, this function is invoked. It calls the receive function of
             * the other Link making up the PCIELink. 
             *
             * @param pkt The packet that has finished transmission across the 
             * unidirectional link and needs to be sent to the opposing link 
             * interface. 
             */
            void txComplete(PCIELinkPacket * pkt) ;

            /**
             * Function called on expiry of replay timer. Initiates a retransmission
             * of the first PCIELinkPAcket in the replay buffer. 
             * 
             */
            void timeoutFunc() ;
    
            /**
             * Function called on expiry of Ack Timer. This function checks if there
             * are any unAcked TLPs that have been received. If so, it sends an 
             * Ack to the opposite interface, and updates the value of last_acked.
             */
            void sendAck() ;
    

            /**
             * Returns the name of this Link. Used for statistics and printing out
             * config and debugging info. 
             */ 
            const std::string name() const { return __name ; }
        
            /**
             * Link constructor called during creation of a Link instance.
             *
             * @param _name: Name of this Link
             *
             * @param p: Pointer to PCIELink which this Link is a part of
             * 
             * @param rate The number of ticks to transmit a byte. The value of 
             * ticksperByte. 
             *
             * @param delay The propagation delay of a packet transmitted across 
             * a unidrectional link
             * 
             * @param delay_var The variance in delay. 
             *
             * @param master_port_src The LinkRequestPort attached to this Link's 
             interface.
            * 
            * @param slave_port_src The LinkResponsePort attached to this Link's 
            interface.
            * 
            * @param master_port_dest The LinkRequestPort attached to the opposite
            * interface.
            * 
            * @param slave_port_dest  The LinkResponsePort attached to the opposite 
            * interface.
            * 
            * @param num_lanes The number of lanes in the unidirectional link.
            * 
            * @param mps : The maximum payload size of a TLP tranmsiited across
            the unidirectional link. Used to calculate retryTime.
            * 
            * @param opposite_link A pointer to the other link making up the 
            *        PCIELink
            */ 
                
            Link (const std::string & _name, PCIELink * p, double rate, Tick delay, 
                Tick delay_var, LinkRequestPort * master_port_src , 
                LinkResponsePort * slave_port_src, LinkRequestPort * master_port_dest, 
                LinkResponsePort * slave_port_dest, int num_lanes,int max_queue_size,  
                int mps, Link ** opposite_link );
        } ; 
    
        /**
         * Class that represents the Request Port attached to a Link. This Request
         * Port is connected to the Response Port of a device attached to one end of 
         * the PCIELink. 
         */ 
        class LinkRequestPort : public RequestPort 
        {  
        public:
            /**
             * Called by the peer Response Port belonging to a connected device to 
             * send a Gem5 packet to the Link Request Port. 
             *
             * @param pkt The Gem5 packet to be received. 
             */    
            bool recvTimingResp(PacketPtr pkt) ;
            
            void recvFromLink(PCIELinkPacket * pkt){} ;
    
            void recvReqRetry(){}
    
            /**
             * Pointer to parent PCIELink
             */
            PCIELink * parent ;

            /** 
             * Double Pointer to the Link this LinkRequestPort is attached to.
             */
            Link ** transmitLink ;
    
            /**
             * Constructor
             * @param _name The name of this LinkRequestPort
             * @param parent_ptr The parent PCIELink. 
             * @param transmit_link_dptr The Link this LinkRequestPort is attached to
             */
            LinkRequestPort(const std::string & _name, PCIELink * parent_ptr,  
                        Link ** transmit_link_dptr) ;   
        } ;
        
        /**
         * Class that represents the Response Port attached to a Link. This Response
         * Port is connected to the Request Port of a device attached to one end of 
         * the PCIELink. 
         */ 
        class LinkResponsePort : public ResponsePort
        {   
        public:

            void recvRespRetry(){}

            /**
             * Since PCI Express consists of a serial link, and not a bus, there is
             * no need for address mapping of devices connected to the link. 
             *
             * @retval Returns an empty AddrRangeList
             */
            AddrRangeList getAddrRanges() const { AddrRangeList temp ; return temp;}
            
            /**
             * Function called by the peer Request Port belonging to a connected PCI
             * device or switch/root port. This functions accepts a timing request
             * packet
             *
             * @param pkt Timing Req packet which is received.  
             */ 
            bool recvTimingReq(PacketPtr pkt) ;

        
            /**
             * Function to receive a functional request packet from peer MAster Port
             * Just passes packet along without modelling timing or delay
             */
            void recvFunctional(PacketPtr pkt) ; 

            /** Function to receive an atomic request packet from peer Response Port. 
             *  Just passes packet along without modelling timing or delay
             */
            Tick recvAtomic(PacketPtr pkt) ;

            /**
             * Pointer to parent PCIELink
             */
            PCIELink * parent ;
            
            /**
             * Double Ptr to Link this LinkResponsePort is attached to.
             */
            Link ** transmitLink ;
    
            /**
             * Constructor. 
             * @param _name Name of the LinkResponsePort instance
             * @param _parent pointer to parent PCIELink. 
             * @param transmit_link_dptr Double Ptr to the Link this LinkResponsePort  
             * is attached to.
             */
            LinkResponsePort(const std::string & _name, PCIELink * parent_ptr,  
                        Link ** transmit_link_dptr) ;   
        } ;

        public: 
        /**
         * Upstream and Downstream Response ports used to connect the PCIELink to the
         * Request Ports of connected devices.Upstream refers to direction of CPU
         * and downstream away from the CPU. One slave port is assigned to each 
         * of the 2 Links that make up the PCIElink.  
         */  
        LinkResponsePort upstreamResponse, downstreamResponse ; 
        
        /**
         * Upstream and Downstream Request ports used to connect the PCIELink to the
         * Response Ports of connected devices. Upstream refers to direction of CPU
         * and downstream away from the CPU. One master port is assigned to each of
         * the 2 Links that make up the PCIELink.
         */  
        LinkRequestPort upstreamRequest , downstreamRequest ;

        /**
         * Representing the 2 Links that make up a PCIELink. These are created 
         * on the heap.
         */
        Link * links[2] ;                                   

        /**
         * Check connectivity of all 4 ports belonging to the PCIELink
         */
        void init() ;
    
        /**
         * Returns a reference to the LinkRequestPort based on name
         */
        LinkRequestPort& getRequestPort(const std::string &if_name, PortID idx) ;
    
        /**
         * Returns a reference to a LinkResponsePort based on name
         */
        LinkResponsePort&  getResponsePort(const std::string &if_name, PortID idx) ;
        Port & getPort(const std::string &if_name, PortID idx=InvalidPortID) override;
        /**
         * Constructor for PCIELink class. 
         * Dynamically creates the 2 Links that make up a PCIELink. 
         * @param p Used to initialize the PCIELink based on python configuration
         */
        PCIELink (const PCIELinkParams & p) ; 
    } ; 
}
 
#endif //__DEV_PCIE_LINK_HH__

