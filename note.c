#include "note.h"
#include "math.h"
#include <stdio.h>

int octave = 7;
int currNote;
int currOct;
int currFreq;
int freqOffset;
int cents;

//finds and prints note of frequency and deviation from note
short findNote(float f, int a4) {
	float c=a4*c_scale_factor;
	float a, b, r;
	int premod, n;
	int oct=4;
	short note=0;
	//determine which octave frequency is in
	if(f >= c) {
		while(f > c*2) {
			c=c*2;
			oct++;
		}
	}
	else { //f < C4
		while(f < c) {
			c=c/2;
			oct--;
		}
	
	}

	//find note below frequency
	//c=middle C
	r=c*root2;
	while(f > r) {
		c=c*root2;
		r=r*root2;
		note++;
	}

	a = (f+freqOffset)/a4;
	b = 12*log2(a)+49;
	n = (int)round(b);
	premod = (n-1) % 12;
	note = premod;

	float realF = a4*pow(root2,(n-49));

	cents = (int)round(1200*log2(f/realF));
//	xil_printf("cents: %d\r\n", cents);


	//determine which note frequency is closest to
	if((f-c) <= (r-f)) { //closer to left note
//		WriteString("N:");
//		WriteString(notes[note]);
//		WriteInt(oct);
//		WriteString(" D:+");
//		WriteInt((int)(f-c+.5));
//		WriteString("Hz");
	}
	else { //f closer to right note
		note++;
		if(note >=12) note=0;
//		WriteString("N:");
//		WriteString(notes[note]);
//		WriteInt(oct);
//		WriteString(" D:-");
//		WriteInt((int)(r-f+.5));
//		WriteString("Hz");
	}

	recordOctave(oct);

	return note;
}

void recordOctave(int oct) {
	octave = oct;
}
