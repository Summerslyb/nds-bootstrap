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

#include <nds.h> 
#include <nds/fifomessages.h>
#include "sdmmc.h"
#include "debugToFile.h"
#include "fat.h"

static bool initialized = false;
static bool initializedIRQ = false;
extern vu32* volatile cardStruct;
extern vu32* volatile cacheStruct;
extern u32 fileCluster;
extern u32 sdk_version;
vu32* volatile sharedAddr = (vu32*)0x027FFB08;

void runCardEngineCheck (void) {
	//dbg_printf("runCardEngineCheck\n");
	int oldIME = enterCriticalSection();
	
	if(!initialized) {
		if (sdmmc_read16(REG_SDSTATUS0) != 0) {
			sdmmc_controller_init();
			sdmmc_sdcard_init();
		}
		FAT_InitFiles(false);
		u32 myDebugFile = getBootFileCluster ("NDSBTSRP.LOG");
		enableDebug(myDebugFile);
		dbg_printf("logging initialized\n");		
		dbg_printf("sdk version :");
		dbg_hexa(sdk_version);		
		initialized=true;
	}

	if(*(vu32*)(0x027FFB08) == (vu32)0x027FEE04)
    {
        dbg_printf("\ncard read received\n");	
			
		// old sdk version
		u32 src = *(vu32*)(sharedAddr+1);
		u32 dst = *(vu32*)(sharedAddr+2);
		u32 len = *(vu32*)(sharedAddr+3);
		
		dbg_printf("\nstr : \n");
		dbg_hexa(cardStruct);		
		dbg_printf("\nsrc : \n");
		dbg_hexa(src);		
		dbg_printf("\ndst : \n");
		dbg_hexa(dst);
		dbg_printf("\nlen : \n");
		dbg_hexa(len);
		
		fileRead(0x027ff800 ,fileCluster,src,len);
		
		dbg_printf("\nread \n");
		
		*sharedAddr = 0;
		
	}
	
	if(*(vu32*)(0x027FFB08) == (vu32)0x027FEE05)
    {
        dbg_printf("\ncard read receivedv2\n");
		
		// old sdk version
		u32 src = *(vu32*)(sharedAddr+1);
		u32 dst = *(vu32*)(sharedAddr+2);
		u32 len = *(vu32*)(sharedAddr+3);
		
		dbg_printf("\nstr : \n");
		dbg_hexa(cardStruct);		
		dbg_printf("\nsrc : \n");
		dbg_hexa(src);		
		dbg_printf("\ndst : \n");
		dbg_hexa(dst);
		dbg_printf("\nlen : \n");
		dbg_hexa(len);
		
		fileRead(dst,fileCluster,src,len);
		
		dbg_printf("\nread \n");
		
		*sharedAddr = 0;
		
	}

	leaveCriticalSection(oldIME);
}

//---------------------------------------------------------------------------------
void myIrqHandlerFIFO(void) {
//---------------------------------------------------------------------------------
	nocashMessage("myIrqHandlerFIFO");
	
	runCardEngineCheck();
}


void myIrqHandlerVBlank(void) {
	nocashMessage("myIrqHandlerVBlank");
	
	runCardEngineCheck();
}

void ipcSyncEnable() {	
	dbg_printf("ipcSyncEnable\n");
	REG_IPC_SYNC |= IPC_SYNC_IRQ_ENABLE;
	nocashMessage("IRQ_IPC_SYNC enabled");
	REG_IE |= IRQ_IPC_SYNC;
}

