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
 * Definition of a PCI Express Link
 */



#include "dev/pcie_link.hh"
#include <cmath>
#include "base/random.hh"
#include "base/trace.hh"

#include "base/inifile.hh"
#include "base/intmath.hh"
#include "base/str.hh"

#include "sim/core.hh"
#include "sim/serialize.hh"
#include "sim/system.hh"


namespace gem5
{
    PCIELinkPacket :: PCIELinkPacket()
    {
        // Initialize a PCIELinkPacket with null and zero values
        pkt = NULL ;
        dllp = NULL ; 
        isDLLP = false ; 
        isTLP = false ;
        seqNum = 0 ;
    }

    int 
    PCIELinkPacket :: getSize()
    {
        // if pkt != NULL, this PCIELinkPacket encapsulates a TLP (Gem5 packet). 
        // Calculate size accordingly taking overheads and payload size into account

        if (pkt != NULL) {
            if (pkt->isRequest() && pkt->hasData()) {
                return (PCIE_REQUEST_HEADER_SIZE + pkt->getSize() + DLL_OVERHEAD
                        + PHYSICAL_OVERHEAD) * ENCODING_FACTOR ; 

            } else if (pkt->isRequest()) {
                return (PCIE_REQUEST_HEADER_SIZE + DLL_OVERHEAD + 
                        PHYSICAL_OVERHEAD) * ENCODING_FACTOR ;

            } else if (pkt->isResponse() && pkt->hasData()) {
                return (PCIE_RESPONSE_HEADER_SIZE + pkt->getSize() + 
                        DLL_OVERHEAD + PHYSICAL_OVERHEAD) * ENCODING_FACTOR ;
            } else {
                return (PCIE_RESPONSE_HEADER_SIZE  + DLL_OVERHEAD + 
                        PHYSICAL_OVERHEAD) * ENCODING_FACTOR ;
            }
        }

        // if dllp != NULL, this PCIELinkPacket encapsulates an ACK DLLP   
        if (dllp != NULL)
            return (DLLP_SIZE + PHYSICAL_OVERHEAD) * ENCODING_FACTOR  ; 
        return 0 ; 
    }


    ReplayBuffer :: ReplayBuffer (int max_size)
    {
        // Initialize the buffer with null ptrs, indicating an empty buffer
        maximumSize = max_size ;
        for (int i = 0 ; i < max_size ; i++)
            queue.push_back(NULL) ;
    }

    int 
    ReplayBuffer::size()
    {
        // Count the number of entries in the replay buffer by counting the 
        // number of non-null ptrs stored.
        int count = 0 ;
        for (std::deque<PCIELinkPacket*>::iterator it = queue.begin() ; 
            it != queue.end() ; it++) {
    
            if ((*it) != NULL) count++ ;
        } 
        return count ;
    }


    void
    ReplayBuffer::pushBack(PCIELinkPacket * ptr)
    {
        // Store a PCIELinkPacket ptr at the back of the replay buffer. 
        if (size() >= maximumSize) 
            return;
        for (std::deque<PCIELinkPacket*>::iterator it = queue.begin() ; 
                it != queue.end() ; it++) {
        
            if (*it == NULL) {
                *it = ptr ; 
                break ; 
            }
        }
    } 

    PCIELinkPacket * 
    ReplayBuffer :: popFront()
    {
        // Remove and return the PCIELinkPacket ptr stored at the front of the buff. 
        if (queue.front() == NULL) 
            return NULL ;
    
        PCIELinkPacket * temp = queue.front() ;   
        queue.pop_front() ; 
        queue.push_back(NULL) ; 
        return temp ;  
    }


    PCIELinkPacket *
    ReplayBuffer :: get (int idx)
    {
        // Get the PCIELinkPacket ptr stored at the idx position in the buffer.
        if (idx > maximumSize) 
            return NULL ;
        return queue[idx] ; 
    }

    bool 
    ReplayBuffer::empty()
    {
        // Is the buffer empty ? Check for non-null values stored in it. 
        for (std::deque<PCIELinkPacket*>::iterator it = queue.begin() ; 
            it != queue.end() ; it++) {
            
            if (*it != NULL) 
                return false ; 
        }
        return true ;

    } 

    PCIELink :: Link :: Link(const std::string & _name, PCIELink *p, 
                            double rate, Tick delay, Tick delay_var, 
                            LinkRequestPort * master_port_src, 
                            LinkResponsePort *  slave_port_src, 
                            LinkRequestPort * master_port_dest, 
                            LinkResponsePort *  slave_port_dest, 
                            int num_lanes, int max_queue_size, 
                            int mps, Link ** opposite_link) 
        
        : __name(_name), parent(p), ticksPerByte(rate), linkDelay(delay), 
        delayVar(delay_var), masterPortSrc(master_port_src), 
        slavePortSrc(slave_port_src), 
        masterPortDest(master_port_dest), 
        slavePortDest(slave_port_dest), 
        lanes(num_lanes), packet(NULL), doneEvent([this]{txDone() ; } , _name), 
        txQueueEvent([this]{processTxQueue();} , _name), 
        timeoutEvent([this]{timeoutFunc() ; }, _name), 
        ackEvent([this]{sendAck() ; }, _name), 
        buffer(max_queue_size), sendSeqNum(0), recvSeqNum(0), lastAcked(0), 
        maxQueueSize(max_queue_size), retryResp(false), retryReq(false), 
        replayPacket(NULL), replayDLLP(NULL), retransmit(false), 
        retransmitIdx(0), otherLink(opposite_link)
    {
        // Assign ack_factor based on number of lanes.
        float ack_factor = 1.4 ; 
        if (lanes == 8) {
            ack_factor = 2.5 ;
        } 
        
        if (lanes >=12) {
            ack_factor = 3.0 ;
        } 
    
        // Calculate the retryTime based on the formula in pcie_link.hh
        
        // Time to transmit a byte on an x1 link ; ticksperByte * ENCODING_FACTOR
        float symbol_time = ticksPerByte * ENCODING_FACTOR  ; 
        
        // assume internal delay of 0 and ack factor of 1.4/2.5/3.0                                                
        retryTime = (((mps + PCIE_REQUEST_HEADER_SIZE + DLL_OVERHEAD + 
                        PHYSICAL_OVERHEAD)*3*ack_factor)/lanes) * symbol_time ;   

    }

    PCIELink::LinkRequestPort::LinkRequestPort (const std::string & _name, 
                                            PCIELink * parent_ptr, 
                                            Link ** transmit_link_dptr) 
    
        : RequestPort(_name, parent_ptr), 
        parent(parent_ptr),
        transmitLink(transmit_link_dptr) 
                                                                                                                                    
    {}
        

    PCIELink::LinkResponsePort::LinkResponsePort (const std::string & _name, 
                                            PCIELink * parent_ptr, 
                                            Link ** transmit_link_dptr) 
        : ResponsePort(_name, parent_ptr), 
        parent(parent_ptr), 
        transmitLink(transmit_link_dptr)  
                                                
    {} 



    PCIELink :: PCIELink(const PCIELinkParams &p) 
        
        : SimObject(p), 
        upstreamResponse(p.name +".upstream_slave", this, &(this->links[0])), 
        downstreamResponse(p.name +".downstream_slave", this, &(this->links[1])), 
        upstreamRequest(p.name + ".upstream_master", this , &(this->links[0])), 
        downstreamRequest(p.name +".downstream_master", this, &(this->links[1]))
    {
        // Dynamically create the 2 Links that make up the PCIELink
    
        links[0] = new Link (p.name + ".down_link", this, p.speed, p.delay, 
                            p.delay_var, (LinkRequestPort*)&upstreamRequest, 
                            (LinkResponsePort*) & upstreamResponse, 
                            (LinkRequestPort*)&downstreamRequest, 
                            (LinkResponsePort*)&downstreamResponse, 
                            p.lanes, p.max_queue_size, p.mps,
                            (Link**)&(this->links[1])) ;
    
    links[1] = new Link  (p.name + ".up_link", this, p.speed, p.delay, 
                            p.delay_var, (LinkRequestPort*)&downstreamRequest, 
                            (LinkResponsePort*)&downstreamResponse, 
                            (LinkRequestPort*)&upstreamRequest, 
                            (LinkResponsePort*)&upstreamResponse, 
                            p.lanes, p.max_queue_size, p.mps, 
                            (Link**)&(this->links[0])) ;

    }
    
    bool
    PCIELink::Link::transmit(PCIELinkPacket * pkt)
    {
        if (busy()) {
            DPRINTF(PCIELink, "packet not sent, link busy\n");
            return false;
        }

        DPRINTF(PCIELink, "packet sent: len=%d\n", pkt->getSize());

        packet = pkt;
        
        // calculate the time to transmit a PCIELinkPacket on the unidirectional
        // link based on the configured bandwidth and number of lanes

        Tick delay = (Tick)ceil(((double)pkt->getSize()*(ticksPerByte/lanes))+1.0);
        if (delayVar != 0)
            delay += random_mt.random<Tick>(0, delayVar);

        DPRINTF(PCIELink, "scheduling packet: delay=%d, (rate=%f)\n", delay, 
            ticksPerByte);

        // schedule the done event after a time corresponding to the time taken
        // to transmit a packet on the unidirectional link
        parent->schedule(doneEvent, curTick() + delay);

        return true;
    }

    void
    PCIELink::Link::txDone()
    {

        // If there is a delay assigned to the unidirectional link, queue the packet
        // again till the delay time gets over. Used to model propagation delay
        if (linkDelay > 0) {
            
            DPRINTF(PCIELink, "packet delayed: delay=%d\n", linkDelay);
            txQueue.emplace_back(std::make_pair(curTick() + linkDelay, packet));
            if (!txQueueEvent.scheduled())
                parent->schedule(txQueueEvent, txQueue.front().first);
        } else {
            assert(txQueue.empty());
            txComplete(packet);
        }
        
        PCIELinkPacket * _packet = packet ;
        
        // This packet has finished transmission. Hence set it as NULL.  
        packet = NULL ;

        assert(!busy());
    
        // If an Ack DLLP id pending, transmit the Ack. Acks have  highest priority.
        if (replayDLLP != NULL) {               
            bool flag = transmit(replayDLLP) ; 
            if (flag) {
                replayDLLP = NULL ;
            } else {
                return ; // This return should never take place. Add sanity checks.
            }   
        } 

        // If a TLP has been transmitted for the first time, start the replay timer.
        // Also store the corresponding PCIELinkPAcket in the replay buffer.
        // If any TLPs were unable to be accepted by the link interface due to an
        // occupied link, make the attached device send them again. 
    
        if ( _packet == replayPacket) {

            buffer.pushBack(_packet) ; 
            replayPacket = NULL ;
            if (!timeoutEvent.scheduled()) {
                parent->schedule(timeoutEvent , curTick() + retryTime) ;
            } 
            if (retryReq) {  
                retryReq = false ;  
                slavePortSrc->sendRetryReq();
            }
            if (retryResp) { 
                retryResp = false ; 
                masterPortSrc->sendRetryResp() ;
            }  
        } 


        // Retransmit a packet from the replay buffer if the replay timer has
        // expired. Increment retransmitIdx so the next packet in replay buffer is 
        // retransmitted. 
    
        if (retransmit) 
        {
            
            if (!timeoutEvent.scheduled()) {
                parent->schedule(timeoutEvent , curTick() + retryTime) ; 
            }

            if (retransmitIdx >= buffer.size()) {
                retransmitIdx = 0 ;
                retransmit = false ; 
                if (retryReq) { 
                    retryReq = false ; 
                    slavePortSrc->sendRetryReq();
                }
                if(retryResp) {
                    retryResp = false ; 
                    masterPortSrc->sendRetryResp() ; 
                }
            } else {
                transmit(buffer.get(retransmitIdx) ) ; 
                retransmitIdx ++ ; 
                return ; 
            }
        }

        // Transmit pending TLP if any.
        if (replayPacket != NULL) {
            transmit(replayPacket) ;
        }     
                
    }

    void
    PCIELink::Link::processTxQueue()
    {
        auto cur(txQueue.front());
        txQueue.pop_front();  

        // Schedule a new event to process the next packet in the queue.
        if (!txQueue.empty()) {
            auto next(txQueue.front());
            assert(next.first > curTick());
            parent->schedule(txQueueEvent, next.first);
        }

        assert(cur.first == curTick());
        txComplete(cur.second);
    }

    void
    PCIELink::Link::txComplete(PCIELinkPacket * packet)
    {
        // Call the receive function of the opposite link interface and pass
        // the packet to it.
        DPRINTF(PCIELink, "packet received: len=%d\n", packet->getSize());
    
        (*otherLink)->linkReceiveInterface(packet) ; 
    }
        

    

    void 
    PCIELink :: Link :: linkReceiveInterface ( PCIELinkPacket * packet)
    {
        
        if (packet->isTLP) {
            
            // If a TLP is received, check its sequence number. Attempt to send
            // the Gem5 packet to the attached device, if the sequence  number 
            // equals recvSeqNum. This check preserves order of packets across
            // the link. 
            if (packet->seqNum == recvSeqNum) {
            
                bool success = (packet->pkt->isResponse()) ? 
                            slavePortSrc->sendTimingResp(packet->pkt) : 
                            masterPortSrc->sendTimingReq(packet->pkt) ; 
                if (!success) 
                    return ;
                
                recvSeqNum ++ ;
            }
        
            if (!ackEvent.scheduled()) {
                
                // If Gem5 packet is successfully sent to the attached device, 
                // return an Ack to the interface which sent this packet.
                PCIELinkPacket * _packet = new PCIELinkPacket ; 
                _packet->isDLLP = true ; 
                _packet->dllp = new DataLinkPacket(recvSeqNum -1) ; 
    
                bool _transmit = transmit(_packet) ; 
                if (!_transmit) {
                    replayDLLP = _packet ; 
                    lastAcked = recvSeqNum - 1 ; 
                    parent->schedule(ackEvent, curTick() + retryTime/3) ; 
                }
            }
            
            return ; 

    } else if (packet->isDLLP) {

            // If an Ack is received, first reset and hold the replay timer. 
            // Next remove all PCIELinkPackets from the replay buffer that 
            // encapsulate a TLP with a sequence number <= Ack sequence num.

            uint64_t seq_num = packet->dllp->seqNum ; 
            if (timeoutEvent.scheduled()) 
                parent->deschedule(timeoutEvent) ; 

            bool flag = false ; 
            while (!flag) {
                PCIELinkPacket * temp = buffer.front() ; 
                if (temp == NULL) { 
                    flag = true ; 
                } else if (temp->seqNum <= seq_num) {
                    buffer.popFront() ;
                    delete temp ; 
                } else {
            
                    flag = true ; 
                    parent->schedule(timeoutEvent , curTick() + retryTime) ;
                }
            }
        
            delete packet->dllp ;
            delete packet ; 
            if (retryReq) {
                retryReq = false ;
                slavePortSrc->sendRetryReq() ;
            }
            if (retryResp) {
                retryResp = false ; 
                masterPortSrc->sendRetryResp() ;
            }
        } 
    }   
        

        

    bool PCIELink::Link::linkTransmitInterface(PacketPtr pkt , bool master)
    {
        // If replay buffer is full or if a retransmission is in
        // place, reject the packet. 
        // replayPacket is non-null if either a TLP is waiting to be 
        // transmitted on the link , or if a TLP is currently being transmitted
        // on the link. 
        if (replayPacket !=  NULL || buffer.size() >= maxQueueSize || 
        retransmit == true) {
            if (!master) {
                retryReq = true ;
            } else {
                retryResp = true ;
            } 
        return false ; 
    }

        // Encapsulate the Gem5 packet within a PCIELinkPAcket and assign it a 
        // sequence number. Transmit the PCIELinkPacket on the unidirectional link. 
        PCIELinkPacket * _packet = new PCIELinkPacket ; 
        _packet->isTLP = true ; 
        _packet->pkt = pkt ; 
        _packet->seqNum = sendSeqNum ; 
        replayPacket = _packet ; 
        transmit(_packet) ;
        sendSeqNum ++ ;
        return true ; 
    }
    
    

    bool 
    PCIELink :: LinkResponsePort :: recvTimingReq(PacketPtr pkt) 
    {  
        // calls the transmit function of the link interface attached to the port.
        return (*transmitLink)->linkTransmitInterface(pkt , false) ; 
    }

    Tick 
    PCIELink :: LinkResponsePort :: recvAtomic(PacketPtr pkt) 
    {  
        (*transmitLink)->masterPortDest->sendAtomic(pkt) ;
        
        // Timing of atomic requests not modelled. 
        return 0 ;
    } 

    void 
    PCIELink :: LinkResponsePort :: recvFunctional(PacketPtr pkt) 
    {  
        // Treat functional requests the same as timing ones ?
        recvTimingReq(pkt); 
    } 

    bool 
    PCIELink :: LinkRequestPort :: recvTimingResp(PacketPtr pkt) 
    {  
        // calls the transmit function of the link interface attached to port.
        return (*transmitLink)->linkTransmitInterface(pkt , true) ; 
    }


    void 
    PCIELink :: Link ::timeoutFunc()
    { 
        if (buffer.size() == 0 || buffer.front() == NULL || retransmit == true) {
            return ;
        }
    
        // Attempt to initiate a retransmission by retransmitting the first packet
        // from the replay buffer.
        bool success = transmit(buffer.front()) ;
        retransmit = true ; 
        retransmitIdx = (success) ? 1 : 0 ;
    }



    void 
    PCIELink :: Link::sendAck()
    {
        // Check if any unacked , but successfully received packets exist. 
        if (lastAcked == recvSeqNum - 1 ) 
            return ;  

        // Create an Ack DLLP with the sequence number equivalent to the sequence
        // number of most recently received unAcked packet.
        PCIELinkPacket * _packet = new PCIELinkPacket ; 
        _packet->isDLLP = true ;
        _packet->dllp = new DataLinkPacket(recvSeqNum - 1) ;
        lastAcked = recvSeqNum - 1 ; 
        parent->schedule(ackEvent , curTick() + retryTime/3) ;
        bool success = transmit(_packet) ; 
        if (!success)
            replayDLLP = _packet ;  
    }


    void 
    PCIELink::init()
    {
        // if (!upstreamRequest.isConnected() || !upstreamResponse.isConnected() || 
        // !downstreamRequest.isConnected() || !downstreamResponse.isConnected()) {
        
        //     fatal ("PCIE Link Ports must be connected !!\n") ;
        // } 
    }


    PCIELink::LinkRequestPort&
    PCIELink::getRequestPort(const std::string &if_name, PortID idx)
    {
        if (if_name == "downstreamRequest") {
            return downstreamRequest;
        } else if (if_name == "upstreamRequest") {
            return upstreamRequest ; 
        }
        else
            fatal("%s does not have any port named %s\n", name(), if_name);
    }

    PCIELink::LinkResponsePort&
    PCIELink::getResponsePort(const std::string &if_name, PortID idx)
    {
        if (if_name == "upstreamResponse") {
            return upstreamResponse;
        } else if (if_name == "downstreamResponse") {
            return downstreamResponse ;
        }
        else
            fatal("%s does not have any port named %s\n", name(), if_name);
    }
    Port & PCIELink::getPort(const std::string &if_name, PortID idx)
    {
        if (if_name == "downstreamRequest") {
            return downstreamRequest;
        } else if (if_name == "upstreamRequest") {
            return upstreamRequest ; 
        }
        else if (if_name == "upstreamResponse") {
            return upstreamResponse;
        } else if (if_name == "downstreamResponse") {
            return downstreamResponse ;
        }
        else
            fatal("%s does not have any port named %s\n", name(), if_name);
    }

}



 
  
      
  
   












   
    

