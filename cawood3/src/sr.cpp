#include "../include/simulator.h"
#include <cstring>
#include <cstdio>
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
  int deltaTime;

  bool wasSent;
  bool wasAckd;
};

float TIMEOUT = 30;
int ASeqnumFirst = 0;           // SeqNum of first frame in window. Same as send_base
int ASeqnumN = 0;               // SeqNum of Nth frame in window. Same as nextseqnum
bool timerInUse = false;        // Ensures we only use one timer at a time
std::vector<pktData> packets;   // To store all frames of data. This acts as our sender view

int BRcvBase = 0;               // Expected SeqNum of first frame in receiver window. Same as rcv_base
int BRcvN = 0;
std::vector<pkt> recvBuffer;    // Buffer of received packets. Will help deliver consecutively numbered packets

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
    if(!timerInUse) {
      timerInUse = true;
      starttimer(A, TIMEOUT);
    }
  }
  packets.push_back(pktDat); // Add packet to our collection
  ASeqnumN++; // Increase upper limit of window regardless, since we know packets buffered or not will get sent regardless
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
  if(getChecksum(packet) == packet.checksum) {
    // if(packet.seqnum - ASeqnumFirst < getwinsize()) {
    //   packets[packet.seqnum].wasAckd = true; // Mark as recv'd
      if(packet.seqnum == ASeqnumFirst) {
        // If packets seqnum is equal to the base, move up the base to the unackd packet with the smallest seq number
        // while(!packets[ASeqnumFirst].wasAckd)
        ASeqnumFirst++;
        stoptimer(A);
      }
    // }
  }
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
  tolayer3(A, packets[ASeqnumFirst].packet);       // First, resend the base packet since the timer is tied in to the base
  packets[ASeqnumFirst].timeSent = get_sim_time(); // Update starttime for packet
  starttimer(A, TIMEOUT);                          // Restart timer
  // TODO: Account for retransmitting multiple packets
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
  char msg[20];
  strncpy(msg, packet.payload, 20);
  if(getChecksum(packet) == packet.checksum) {
    // Handle two cases, where seqNum is in [BRcvBase, BRcvBase+N-1], or in [BRcvBase-N, BRcvBase-1]
    if(BRcvN - BRcvBase < getwinsize()) {
      pkt ack = makePkt(msg, -1, packet.seqnum); // Create ACK
      recvBuffer[packet.seqnum] = ack;           // Buffer ACK
      tolayer3(B, ack);                          // Send ACK
      // Send packet to upper layer if the seqnum is rcv_base
      if(BRcvN == packet.seqnum) {
        tolayer5(B, packet.payload);
        BRcvN++;
        BRcvBase++;
        // As well as any other packets in [rcv_base, rcv_base+N-1]
      }
    }
  }
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
  // Fill recv buffer with empty packets
  for(int i=0; i<1000; i++) {
    pkt pack;
    pack.seqnum = -1;
    pack.acknum = -1;
    recvBuffer.push_back(pack);
  }
}
