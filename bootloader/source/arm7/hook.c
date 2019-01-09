/*
    NitroHax -- Cheat tool for the Nintendo DS
    Copyright (C) 2008  Michael "Chishm" Chisholm

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <string.h> // memcpy
#include <stdio.h>
#include <nds/system.h>
#include <nds/debug.h>

#include "hook.h"
#include "common.h"
#include "sdengine_bin.h"

extern unsigned long cheat_engine_size;
extern unsigned long intr_orig_return_offset;

extern const u8 cheat_engine_start[]; 

// libnds v1.5.12 2016
static const u32 homebrewStartSig_2016[1] = {
	0x04000208, 	// DCD 0x4000208
};

static const u32 homebrewEndSig_2016[2] = {
	0x04000004,		// DCD 0x4000004
	0x04000180		// DCD 0x4000180
};

// libnds v_._._ 2007 irqset
static const u32 homebrewStartSig_2007[1] = {
	0x04000208, 	// DCD 0x4000208
};

static const u32 homebrewEndSig2007[2] = {
	0x04000004,		// DCD 0x4000004
	0x04000180		// DCD 0x4000180
};

// interruptDispatcher.s jump_intr:
static const u32 homebrewSig[5] = {
	0xE5921000, // ldr    r1, [r2]        @ user IRQ handler address
	0xE3510000, // cmp    r1, #0
	0x1A000001, // bne    got_handler
	0xE1A01000, // mov    r1, r0
	0xEAFFFFF6  // b    no_handler
};	

// interruptDispatcher.s jump_intr:
//patch
static const u32 homebrewSigPatched[5] = {
	0xE59F1008, // ldr    r1, =0x3900010   @ my custom handler
	0xE5012008, // str    r2, [r1,#-8]     @ irqhandler
	0xE501F004, // str    r0, [r1,#-4]     @ irqsig 
	0xEA000000, // b      got_handler
	0x037C0010  // DCD 	  0x037C0010       
};
 
// accelerator patch for IPC_SYNC
static const u32 homebrewAccelSig[4] = {
	0x2401B510   , // .
	               // MOVS    R4, #1
	0xD0064220   , // .
				// .
	0x881A4B10   , // ...
	0x430A2108   , // ...
};	

/*static const u32 homebrewAccelSigPatched[4] = {
	0x2201B510   , // .
	               // MOVS    R4, #1
	0x43180423   , // LSLS    R3, R4, #0x10 // IRQ IPC SYNC
				   // ORRS    R0, R3        // ENABLE THE BIT
	0x881A4B10   , // ...
	0x430A2108   , // ...
};	*/

static const u32 homebrewAccelSigPatched[4] = {
	0x47104A00   , // LDR     R2, =0x037C0014
	               // BX      R2
	0x037C0020   , // 
				   // 
	0x881A4B10   , // ...
	0x430A2108   , // ...
};

static const int MAX_HANDLER_SIZE = 50;

static u32* hookInterruptHandlerHomebrew (u32* addr, size_t size) {
	u32* end = addr + size/sizeof(u32);
	
	// Find the start of the handler
	while (addr < end) {
		if ((addr[0] == homebrewSig[0]) && 
			(addr[1] == homebrewSig[1]) && 
			(addr[2] == homebrewSig[2]) && 
			(addr[3] == homebrewSig[3]) && 
			(addr[4] == homebrewSig[4])) 
		{
			break;
		}
		addr++;
	}
	
	if (addr >= end) {
		return NULL;
	}
	
	// patch the program
	addr[0] = homebrewSigPatched[0];
	addr[1] = homebrewSigPatched[1];
	addr[2] = homebrewSigPatched[2];
	addr[3] = homebrewSigPatched[3];
	addr[4] = homebrewSigPatched[4];
	
	// The first entry in the table is for the Vblank handler, which is what we want
	return addr;
}

static u32* hookAccelIPCHomebrew (u32* addr, size_t size) {
	u32* end = addr + size/sizeof(u32);
	
	// Find the start of the handler
	while (addr < end) {
		if ((addr[0] == homebrewAccelSig[0]) && 
			(addr[1] == homebrewAccelSig[1]) && 
			//(addr[2] == homebrewAccelSig[2]) && 
			(addr[3] == homebrewAccelSig[3])) 
		{
			break;
		}
		addr++;
	}
	
	if (addr >= end) {
		return NULL;
	}
	
	// patch the program
	addr[0] = homebrewAccelSigPatched[0];
	addr[1] = homebrewAccelSigPatched[1];
	addr[2] = homebrewAccelSigPatched[2];
	addr[3] = homebrewAccelSigPatched[3];
	
	// The first entry in the table is for the Vblank handler, which is what we want
	return addr;
}

int hookNds (const tNDSHeader* ndsHeader, u32* sdEngineLocation, u32* wordCommandAddr) {
	u32* hookLocation = NULL;
	u32* hookAccel = NULL;
	
	nocashMessage("hookNds");

	if (!hookLocation) {
		hookLocation = hookInterruptHandlerHomebrew((u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize);
	}
	
	if (!hookLocation) {
		nocashMessage("ERR_HOOK");
		return ERR_HOOK;
	}
	
	hookAccel = hookAccelIPCHomebrew((u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize);
	
	if (!hookAccel) {
		nocashMessage("ACCEL_IPC_ERR");
	} else {
		nocashMessage("ACCEL_IPC_OK");
	}
	
	memcpy (sdEngineLocation, (u32*)sdengine_bin, sdengine_bin_size);	
	
	sdEngineLocation[1] = myMemUncached(wordCommandAddr);
	
	nocashMessage("ERR_NONE");
	return ERR_NONE;
}

