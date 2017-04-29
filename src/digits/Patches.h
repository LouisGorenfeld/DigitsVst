#ifndef _PATCHES_H_
#define _PATCHES_H_

#define NUMPATCHES 44

extern char presetBank[32][512];

//extern int s_patchLen[NUMPATCHES];

char *s_patchNames[] = { "INIT:Init Tone",
	"PAD: Bell Brass Pad",
	"PAD: Dream Pad",
	"PAD: Flanger Strings",
	"PAD: Ghost Choir",
	"PAD: String Ens.",
	"PAD: String Ens. Bright",
	"PAD: Wintery Pad",

	"SWP: HiSweep",
	"SWP: HiSweep-Overdrive",
	"SWP: Howling Tube",
	"SWP: Mean Sweeps",
	"SWP: Serious",

	"ARP: Eastern Arp",
	"ARP: Future Tines",
	"ARP: House Stabs",
	"ARP: POW!",
	"ARP: Quack! Quack!",
	"ARP: Thunky Arp",
	
	"LEAD:Bell Lead",
	"LEAD:Brass Lead",
	"LEAD:Brassy Slicer",
	"LEAD:Darius",
	"LEAD:Ghosts",
	"LEAD:Harsh Lead",
	"LEAD:Hollow Tube",
	"LEAD:HyperFlute",
	"LEAD:Raving Lunatic",
	"LEAD:ResoLead",
	"LEAD:SIDLead",
	"LEAD:SynReed",
	"LEAD:The Creeps",
	"LEAD:Solo Violin",
	
	"CHRD:Brass Ens.",
	"CHRD:Elec. Organ",
	"CHRD:Hi-Tone",
	"CHRD:Toy Piano",
	
	"BASS:404",
	"BASS:Chorused Bass",
	"BASS:FM Bass",
	"BASS:Flanger Bass",
	"BASS:PWM Bass",
	"BASS:Rubber Band",
	"BASS:Thunky Bass"
};

#endif
