#include "../include/simulator.h"

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

#define SEEKING_ACK false
#define AWAITING_OUT true


float TIMEOUT = 10.00; // timeout param
int SEQNUM; // will always be either 0 or 1 for abt protocol, the SEQNUM for A
bool A_STATE; // when true, awaiting input from layer 5, when false awaiting ACK from layer 3 
int B_SEQNUM; // will always be either 0 or 1 for abt protocol, the SEQNUM for B
struct pkt packetBuffer; // holds the last packet sent so it can be
						 // resent if its lost or corrupted
// ACK seqnum aliasing
int ACK = -1;
// Calling entity value aliasing
int A = 0; 
int B = 1;

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

/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message)
{
	if (A_STATE == AWAITING_OUT) // is A awaiting a message from layer 5?, (A_STATE == true)
	{
		// new packet instance
		struct pkt packet;
		// set seqnum
		packet.seqnum = SEQNUM;

		// set acknum B should reply with
		packet.acknum = SEQNUM;
		
		// package message data into packet payload
		packet.payload = message.data;
		
		int checksum = 0;
		// calculate checksum
		checksum = getChecksum(packet);
		
		// set checksum
		packet.checksum = checksum;

		// start timer A, timeout after TIMEOUT
		starttimer(A,TIMEOUT);

		// buffer sent packet to resend if failed
		packetBuffer = packet;

		// send packet to layer 3
		tolayer3(A,packet);

		// change A_STATE, accepting ACK response from B
		A_STATE = SEEKING_ACK;
	}
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
	int checksum = getChecksum(packet); // calculate checksum
	if (checksum == packet.checksum && A_STATE == SEEKING_ACK) // compare checksums, is packet corrupt? AND Check if awaiting ACK (A_STATE == false)
	{
		// packet is not corrupt, compare seqnum to acknum
		if (packet.acknum == SEQNUM)
		{
			// Stop the timer
			stoptimer(A);

			// Change sequence number
			// if seqnum == 0, set it to 1, else set it to 0
			SEQNUM = (SEQNUM == 0) ? 1:0;
			
			// change A_STATE to receive to messages from layer5
			A_STATE = AWAITING_OUT;
		}
		else
		{
			// if we're here, that means the acknum we got is different from the seqnum
			// do nothing, wait for timerinterupt to resend packet
			return;
		}
	}
	else
	{
		// if we're here, that means the packet was corrupted or A is not waiting for an ACK
		// do nothing, wait for timerinterupt to resend packet
		return;
	}
	
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
	// send copy of packet again
	tolayer3(A,packetBuffer);

	// restart timer
	starttimer(A,TIMEOUT);
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
	SEQNUM = 0;
	A_STATE = AWAITING_OUT;
	
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{
	int checksum = getChecksum(packet); // calculate checksum
	if (checksum == packet.checksum) // compare checksums, is packet corrupt?
	{
		// packet is not corrupt, compare seqnum to acknum
		if (packet.seqnum == B_SEQNUM)
		{
			////////////////////
			// unload packet  //
			
			// extract data from packet
			struct msg message;
			strncpy(message.data, packet.payload, sizeof(packet.payload));

			// deliver message to layer 5
			tolayer5(B,msg);

			/////////////////////
			// send packet ACK //
			
			// new packet instance
			struct pkt packetACK;
			
			// set seqnum to ACK
			packetACK.seqnum = ACK;

			// the seqnum B is acknowledging
			packetACK.acknum = B_SEQNUM;
			
			int checksum = 0;
			// calculate checksum
			checksum = getChecksum(packetACK);
			
			// set checksum
			packetACK.checksum = checksum;

			// send packet to layer 3
			tolayer3(B,packetACK);

			// change to next state
			// if seqnum == 0, set it to 1, else set it to 0
			B_SEQNUM = (B_SEQNUM == 0) ? 1:0;
		}
		else
		{
			// if we're here, that means the acknum we got is different from the seqnum

			// new packet instance
			struct pkt packetACK;
			
			// set seqnum
			packetACK.seqnum = ACK;

			// set acknum B should reply with
			// it should be noted that in this scope, the SEQNUM
			// does not equal the SEQNUM on the A side.
			packetACK.acknum = B_SEQNUM;
			
			int checksum = 0;
			// calculate checksum
			checksum = getChecksum(packetACK);
			
			// set checksum
			packetACK.checksum = checksum;

			// send packet to layer 3
			tolayer3(B,packetACK);
			
		}
	}
	else
	{
		// if we're here, that means the packet was corrupted
		// do nothing, wait for retransmission
		return;
	}
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
	B_SEQNUM = 0;
}
