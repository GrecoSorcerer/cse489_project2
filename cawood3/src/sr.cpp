#include "../include/simulator.h"
#include <iostream>
#include <cstring>
#include <vector>

/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for PA2, unidirectional data transfer
   protocols (from A to B). Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
     (although some can be lost).
**********************************************************************/

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/
#define A 0
#define B 1

// Struct defining packet metadata
struct pktData {
  pkt packet;

  int timeSent;

  bool wasSent;
  bool wasAckd;
};

float TIMEOUT = 50;
int ASeqnumFirst = 0;            // SeqNum of first frame in window. Same as send_base
int ASeqnumN = 0;                // SeqNum of Nth frame in window. Same as nextseqnum
std::vector<pktData> packets;    // To store all frames of data. This acts as our sender view
int it = -1;                     // Ensures physical timer is only called once in A_output

int BRcvBase = 0;                // Expected SeqNum of first frame in receiver window. Same as rcv_base
int BRcvN = 0;
std::vector<pktData> recvBuffer; // Buffer of received packets. Will help deliver consecutively numbered packets

// HELPER FUNCTIONS
int getChecksum(struct pkt packet)
{
	int checksum = packet.seqnum + packet.acknum;
	for (int i=0; i<20; i++) {
		checksum += packet.payload[i];
	}
	return checksum;
}

pkt makePkt(char payload[], int seqnum, int acknum) {
  pkt res;
  strncpy(res.payload, payload, 20);
  res.seqnum = seqnum;
  res.acknum = acknum;
  res.checksum = getChecksum(res);
  return res;
}

pktData makePktData(char payload[], int seqnum, int acknum) {
  pktData res;
  res.packet = makePkt(payload, seqnum, acknum);
  res.timeSent = get_sim_time();
  return res;
}

/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message)
{
  struct pktData pktDat = makePktData(message.data, ASeqnumN, -1); // Construct packet
  if(ASeqnumN - ASeqnumFirst < getwinsize()) {
    pktDat.wasSent = true; // Flag as sent (don't need to set for other case since its initialized to false by default)
    tolayer3(A, pktDat.packet); // Send to layer3, set timer if it hasnt been set
    if(++it < 1) starttimer(A, TIMEOUT); // Start first physical timer. This is only called once.
  }
  packets.push_back(pktDat); // Add packet to our sender view
  ASeqnumN++; // Increase upper limit of window regardless, since we know packets buffered or not will get sent regardless
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
  if(getChecksum(packet) == packet.checksum) {
    packets[packet.seqnum].wasAckd = true; // Mark as recv'd
    if(packet.seqnum == ASeqnumFirst) {
      // If packets seqnum is equal to the base, move up the base to the unackd packet with the smallest seq number
      while(!packets[ASeqnumFirst].wasAckd) ASeqnumFirst++;
      stoptimer(A);
      // Check window for untransmitted packets and retransmit them
      for(int i=ASeqnumFirst; i<ASeqnumN; i++) {
        if(!packets[i].wasSent) {
          packets[i].wasSent = true;
          packets[i].timeSent = get_sim_time();
          tolayer3(A, packets[i].packet);
        }
      }
    }
    // Retransmit any packets that might be expired
    if(ASeqnumFirst < ASeqnumN) {
      for(int i=ASeqnumFirst; i<ASeqnumN; i++) {
        int deltaTime = get_sim_time() - packets[i].timeSent;
        if(deltaTime >= TIMEOUT && !packets[i].wasAckd) {
          packets[i].timeSent = get_sim_time();
          tolayer3(A, packets[i].packet);
        }
      }
    }
  }
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
  packets[ASeqnumFirst].timeSent = get_sim_time(); // First, update starttime for packet
  tolayer3(A, packets[ASeqnumFirst].packet);       // Resend the base packet since the timer is tied in to the base
  starttimer(A, TIMEOUT);                          // Restart timer
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{
  BRcvN =  BRcvBase + getwinsize(); // Update BRcvN
  char msg[20];
  strncpy(msg, packet.payload, 20);
  if(getChecksum(packet) == packet.checksum) {
    // Handle two cases, where seqNum is in [BRcvBase, BRcvBase+N-1], or in [BRcvBase-N, BRcvBase-1]
    if(packet.seqnum <= BRcvN+1 && packet.seqnum >= BRcvBase) {
      // If packet has not been previously received, it is buffered
      if(recvBuffer[packet.seqnum].packet.seqnum == -1) {
        pkt ack = makePkt(msg, packet.seqnum, packet.seqnum); // Create ACK
        recvBuffer[packet.seqnum].packet = ack;               // Buffer ACK
        tolayer3(B, ack);                                     // Send ACK
      }
      // Send packet to upper layer if the seqnum is rcv_base
      if(BRcvBase == packet.seqnum) {
        tolayer5(B, packet.payload);
        recvBuffer[packet.seqnum].wasSent = true; // Flag as being sent (to upper layer)
        // Send any consecutive packets in [rcv_base, rcv_base+N-1]
        for(int i=BRcvBase; i<BRcvN+1; i++) {
          if(!recvBuffer[i+1].wasSent && recvBuffer[i+1].packet.acknum != -1) {
            tolayer5(B, recvBuffer[i+1].packet.payload);
            BRcvBase++;
          } else break;
        }
        BRcvBase++; // Increment once here to account for the initial packet whose ack is the base
      }
    } else if(packet.seqnum <= BRcvBase-1 && packet.seqnum >= BRcvBase - getwinsize()) {
      tolayer3(B, makePkt(msg, packet.seqnum, packet.seqnum));
    }
  }
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
  char tmp[1] = "";
  // Fill recv buffer with empty packets
  for(int i=0; i<1000; i++) {
    recvBuffer.push_back(makePktData(tmp, -1, -1));
  }
}
