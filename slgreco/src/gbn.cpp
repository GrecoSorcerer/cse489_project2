#include "../include/simulator.h"


#include <stdio.h>
#include <string.h>
#include <list>
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
#define A		0
#define B		1
#define PKTMAX  1000
#define ACK     -1

int base;
int nextseqnum;
int windowsize;
float TIMEOUT =  100;

int expectedseqnum;
struct pkt ACKPKT;

std::list<msg> messageBuffer; // To store buffering messages
std::vector<pkt> packetBuffer;  // To store N frames


int getChecksum(struct pkt packet)
{

	int checksum = 0;

	// will likely need to check to make sure this is a valid way to
	// calculate the checksum.

	// checksum seqnum
	checksum += packet.seqnum;
	// checksum acknum
	checksum += packet.acknum;
	// checksum payload
	for (unsigned int i = 0; i < sizeof(packet.payload); i++)
	{
		checksum += (int)packet.payload[i];
	}
	return checksum;
}

pkt makePacket(int seqnum, int acknum, struct msg message)
{
	// new packet instance
	struct pkt packet;

	// set seqnum
	packet.seqnum = seqnum;
	// set acknum
	packet.acknum = acknum;

	// package message data into packet payload
	strncpy(packet.payload, message.data, sizeof(message.data));
	int checksum = 0;
	// calculate checksum
	checksum = getChecksum(packet);

	// set checksum
	packet.checksum = checksum;

	// return the packet
	return packet;
}

int getAckNum(struct pkt packet)
{
	if (packet.acknum == ACK)
	{
		return packet.seqnum;
	}
	return -1;
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

void refuse_data(struct msg message)
{
	// place the refused message into the message buffer until it can be dealt with
	enqueueMsg(message);
	
	printf("\nRefused!!");
	printf((char *)message.data);
}
/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message)
{
	if (nextseqnum < base + windowsize)
	{
		struct pkt packet;

		// make the packet, fill it with data
		packet = makePacket(nextseqnum, 0, message);

		// place the packet into the packet buffer
		packetBuffer[nextseqnum] = packet;

		// send the packet
		tolayer3(A, packet);
		printf("\n> Sending packet to layer 3 from A: ");
		printf((char *)packet.payload);

		// only start the timer at the top of the stack
		if (base == nextseqnum)
		{
			starttimer(A, TIMEOUT);
		}

		nextseqnum++;
	}
	else
	{
		// currently we cant accept more messages, refuse it. handle it elsewhere
		refuse_data(message);
	}
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
	if (packet.checksum == getChecksum(packet))
	{
		base = packet.seqnum + 1;
		printf("\nbase: %i",base);
		printf("\nnext: %i",nextseqnum);
		

		if (base == nextseqnum)
		{
			stoptimer(A);
		}
		else
		{
			stoptimer(A);
			starttimer(A, TIMEOUT);
		}
	}
	else
	{
		// The packet was corrupt
	}

	if (!(messageBuffer.empty()))
	{
		struct pkt packet = makePacket(nextseqnum, 0, dequeueMsg());

		packetBuffer[nextseqnum] = packet;
			
		nextseqnum++;
	}
	
}

/* called when A's timer goes off */
void A_timerinterrupt()
{

	// restart the timer
	printf("\n");
	printf("\nseqnum%i",nextseqnum);
	// resend the window
	
	starttimer(A, TIMEOUT);
	for (int i = base; base < nextseqnum - 1; i++)
	{
		tolayer3(A, packetBuffer[i]);
	}
	
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
	base = 0;
	nextseqnum = 0;
	windowsize = getwinsize();
	packetBuffer.resize(101);
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{
	if (packet.checksum == getChecksum(packet) && packet.seqnum == expectedseqnum)
	{
		// extract the message from the packet and send to next layer
		tolayer5(B, (char *)packet.payload);

		printf("\n>To layer 5 from B: ");
		printf((char *)packet.payload);
		struct msg message;
		ACKPKT = makePacket(expectedseqnum, ACK, message);
		tolayer3(B, packet);
		
		printf("\n<To layer 3 from B: NEW ACKPKT %i",expectedseqnum);

		expectedseqnum++;
	}
	else
	{
		tolayer3(B, ACKPKT);
		printf("\n<To layer 3 from B: LAST ACKPKT");
	}
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
	expectedseqnum = 0;
	struct msg message;
	struct pkt packet = makePacket(expectedseqnum, ACK, message);
	ACKPKT = packet;

}
