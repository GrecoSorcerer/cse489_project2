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

int WINSIZE = getwinsize();
float TIMEOUT = 150.00;

// A Global Vars
int base = 0;
int ASeqnumFirst = 0; // SeqNum of first frame in window
int ASeqnumN = 0;     // SeqNum of Nth frame in window

std::vector<msg> messageBuffer; // To store buffering messages
std::vector<pkt> packetBuffer;  // To store N frames

bool timerUsed = false; // To ensure that we only set up one timer for GBN

// B Global Vars
int BexpectedSeq = 0;

// HELPER FUNCTIONS
int getChecksum(struct pkt packet)
{
	int checksum = packet.seqnum + packet.acknum;
	for (int i=0; i<20; i++) {
		checksum += packet.payload[i];
	}
	return checksum;
}

void enqueueMsg(struct msg message)
{
	messageBuffer.push_back(message);
}

msg dequeueMsg()
{
	struct msg message;
	message = messageBuffer.front();
	messageBuffer.erase(messageBuffer.begin()); // Erase first element

	return message;
}

/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message)
{
  // If the number of unackd packets are less than the window size
  if(ASeqnumN - ASeqnumFirst < WINSIZE) {
    // Construct packet
    struct pkt packet;
    packet.seqnum = ASeqnumN;
    strncpy(packet.payload, message.data, sizeof(message.data));
    packet.checksum = getChecksum(packet);

    // Send to layer3, set timer if it hasnt been set
    tolayer3(A, packet);
    if(!timerUsed) {
      timerUsed = true;
      starttimer(A,TIMEOUT);
    }

    // Update seqnum of nth frame, and add the packet to the buffer
    ASeqnumN++;
    packetBuffer.push_back(packet);
  } else {
    // Buffer message if WINSIZE is full
    enqueueMsg(message);
  }
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{

}

/* called when A's timer goes off */
void A_timerinterrupt()
{
  timerUsed =  false; // our one timer has gone off, so we can use it again
  int i = ASeqnumFirst; // point to oldest unackd packet;
  for(i; i<ASeqnumN; i++) {
    pkt toSend = packetBuffer[i]; // grab packet
    tolayer3(A, toSend);
    if(!timerUsed) {
      starttimer(A, TIMEOUT);
      timerUsed = true;
    }
  }
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

}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
}
