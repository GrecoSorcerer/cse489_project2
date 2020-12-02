#include "../include/simulator.h"
#include <iostream>
#include <cstring>

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
int A = 0;
int B = 1;

int seqNum; // Expected seq value for sender A
int ackNum; // Expected ack value for recv B

int waitForAck; // See if sender A is waiting for ACK or waiting for layer5

pkt lastSentPacket;
float timeOut;


int calculateChecksum(pkt packet) {
  int res = packet.seqnum + packet.acknum;
  for(int i=0; i<20; ++i) {
    res += packet.payload[i];
  }
  return res;
}

/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message)
{
  std::cout << "ATTEMPTING TO SEND MESSAGE: " << message.data << "\n";
  if(waitForAck) return; // Wait for ACK before sending another packet
  char msg[20];
  strncpy(msg, message.data, 20);

  pkt packet;
  packet.seqnum = seqNum;
  strncpy(packet.payload, msg, 20);
  packet.checksum = calculateChecksum(packet);


  lastSentPacket = packet;
  waitForAck = !waitForAck;
  std::cout << "SENT MESSAGE: " << msg << "\n";
  tolayer3(A, packet);
  starttimer(A, timeOut);
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
  if(!waitForAck) return;

  if(packet.acknum == seqNum && packet.checksum == calculateChecksum(packet)) {
    std::cout << "MESSAGE ACK'D: " << packet.payload << "\n";
    std::cout << "FLIPPING BIT FROM " << seqNum << " to " << !seqNum << "\n";
    stoptimer(A);
    seqNum = !seqNum;
    waitForAck = 0;
  }
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
  if(!waitForAck) return;
  std::cout << "TIMEOUT DETECTED. RESENDING " << lastSentPacket.payload << "\n";
  tolayer3(A, lastSentPacket);
  starttimer(A, timeOut);
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
  seqNum = 0;
  waitForAck = 0;
  timeOut = 12;
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{
  char msg[20];
  strncpy(msg, packet.payload, 20);

  int chkSum = calculateChecksum(packet);
  pkt pk;
  pk.checksum = chkSum;

  if(chkSum == packet.checksum && ackNum == packet.seqnum) {
    pk.acknum = ackNum;

    std::cout << "RECEIVED MESSAGE " << msg << "\n";
    std::cout << "FLIPPING BIT FROM " << ackNum << " to " << !ackNum << "\n";

    tolayer5(B, msg); // Send msg to app layer
    tolayer3(B, pk);  // Send ACK to sender A
    ackNum = !ackNum; // Update our ackNum
  } else {
    std::cout << "SEND NACK FOR MESSAGE " << msg << '\n';
    pk.acknum = !ackNum;
    tolayer3(B, pk);
  }
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
  ackNum = 0;
}
