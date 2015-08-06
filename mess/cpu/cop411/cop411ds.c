/****************************************************************************
 *

 *
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "memory.h"


static void InitDasmCOP411(void)
{

}

int DasmCOP411(char *buffer, unsigned pc)
{
	int op;
	int cnt = 1;
	UINT16 addr;
	UINT8 op2;

	op = cpu_readop(pc);

	if ((op >= 0x80 && op <= 0xBE) || (op >= 0xC0 && op <= 0xFE)) {


		if ((pc & 0x3E0) >= 0x80 && (pc & 0x3E0) < 0x100) //JP pages 2,3
		{
			addr = (UINT16)((pc & 0x380) | (op & 0x7F));
			sprintf(buffer,"JP %x",addr);
		}
		else
		{
			if ((op & 0xC0) == 0xC0) //JP other pages
			{
				addr = (UINT16)((pc & 0x3C0) | (op & 0x3F));
				sprintf(buffer,"JP %x",addr);
			}
			else					//JSRP
			{
				addr = (UINT16)(0x80 | (op & 0x3F));
				sprintf(buffer,"JSRP %x",addr);
			}
		}
	}

	switch (op)
	{

		case 0:
	 		sprintf(buffer,"CLRA");
	 		break;

		case 1:
			sprintf(buffer,"SKMBZ 0");
			break;

		case 2:
			sprintf(buffer,"XOR");
			break;

		case 3:
			sprintf(buffer,"SKMBZ 2");
			break;

		case 4:
			sprintf(buffer,"XIS 0");
			break;

		case 5:
			sprintf(buffer,"LD 0");
			break;

		case 6:
			sprintf(buffer,"X 0");
			break;

		case 7:
			sprintf(buffer,"XDS 0");
			break;

		case 8:
			sprintf(buffer,"LBI 0,9 ");
			break;

		case 9:
			sprintf(buffer,"LBI 0,10 ");
			break;

		case 0xA:
			sprintf(buffer,"LBI 0,11 ");
			break;

		case 0xB:
			sprintf(buffer,"LBI 0,12 ");
			break;

		case 0xC:
			sprintf(buffer,"LBI 0,13 ");
			break;

		case 0xD:
			sprintf(buffer,"LBI 0,14 ");
			break;

		case 0xE:
			sprintf(buffer,"LBI 0,15 ");
			break;

		case 0xF:
			sprintf(buffer,"LBI 0,0 ");
			break;

		case 0x11:
			sprintf(buffer,"SKMBZ 1");
			break;

		case 0x13:
			sprintf(buffer,"SKMBZ 3");
			break;

		case 0x14:
			sprintf(buffer,"XIS 1");
			break;

		case 0x15:
			sprintf(buffer,"LD 1");
			break;

		case 0x16:
			sprintf(buffer,"X 1");
			break;

		case 0x17:
			sprintf(buffer,"XDS 1");
			break;

		case 0x18:
			sprintf(buffer,"LBI 1,9");
			break;

		case 0x19:
			sprintf(buffer,"LBI 1,10");
			break;

		case 0x1A:
			sprintf(buffer,"LBI 1,11");
			break;

		case 0x1B:
			sprintf(buffer,"LBI 1,12");
			break;

		case 0x1C:
			sprintf(buffer,"LBI 1,13");
			break;

		case 0x1D:
			sprintf(buffer,"LBI 1,14");
			break;

		case 0x1E:
			sprintf(buffer,"LBI 1,15");
			break;

		case 0x1F:
			sprintf(buffer,"LBI 1,0");
			break;

		case 0x20:
			sprintf(buffer,"SKC");
			break;

		case 0x21:
			sprintf(buffer,"SKE");
			break;

		case 0x22:
			sprintf(buffer,"SC");
			break;

		case 0x23:
			addr = (UINT16)(cpu_readop(pc++) & 0x3F);
			sprintf(buffer,"XAD %x,%x",((addr & 0x30) >> 4),addr & 0x0F);
			cnt = 2;
			break;


		case 0x24:
			sprintf(buffer,"XIS 2");
			break;

		case 0x25:
			sprintf(buffer,"LD 2");
			break;

		case 0x26:
			sprintf(buffer,"X 2");
			break;

		case 0x27:
			sprintf(buffer,"XDS 2");
			break;

		case 0x28:
			sprintf(buffer,"LBI 2,9");
			break;

		case 0x29:
			sprintf(buffer,"LBI 2,10");
			break;

		case 0x2A:
			sprintf(buffer,"LBI 2,11");
			break;

		case 0x2B:
			sprintf(buffer,"LBI 2,12");
			break;

		case 0x2C:
			sprintf(buffer,"LBI 2,13");
			break;

		case 0x2D:
			sprintf(buffer,"LBI 2,14");
			break;

		case 0x2E:
			sprintf(buffer,"LBI 2,15");
			break;

		case 0x2F:
			sprintf(buffer,"LBI 2,0");
			break;

		case 0x30:
			sprintf(buffer,"ASC");
			break;

		case 0x31:
			sprintf(buffer,"ADD");
			break;

		case 0x32:
			sprintf(buffer,"RC");
			break;

		case 0x33:

			op2 = cpu_readop(pc + 1);
			cnt = 2;

			if (op2 >= 0x60 || op2 <= 0x6F) {
				sprintf(buffer,"LEI %x",op2 & 0xF);
			}

			switch (op2) {
				case 0x01:
					sprintf(buffer,"SKGBZ 0");
					break;

				case 0x21:
					sprintf(buffer,"SKGZ");
					break;

				case 0x11:
					sprintf(buffer,"SKGBZ 1");
					break;

				case 0x03:
					sprintf(buffer,"SKGBZ 2");
					break;

				case 0x13:
					sprintf(buffer,"SKGBZ 3");
					break;

				case 0x2A:
					sprintf(buffer,"ING");
					break;

				case 0x2E:
					sprintf(buffer,"INL");
					break;

				case 0x3C:
					sprintf(buffer,"CAMQ");
					break;

				case 0x3E:
					sprintf(buffer,"OBD");
					break;

				case 0x3A:
					sprintf(buffer,"OMG");
					break;

			}
			break;

		case 0x34:
			sprintf(buffer,"XIS 3");
			break;

		case 0x35:
			sprintf(buffer,"LD 3");
			break;

		case 0x36:
			sprintf(buffer,"X 3");
			break;

		case 0x37:
			sprintf(buffer,"XDS 3");
			break;

		case 0x38:
			sprintf(buffer,"LBI 3,9");
			break;

		case 0x39:
			sprintf(buffer,"LBI 3,10");
			break;

		case 0x3A:
			sprintf(buffer,"LBI 3,11");
			break;

		case 0x3B:
			sprintf(buffer,"LBI 3,12");
			break;

		case 0x3C:
			sprintf(buffer,"LBI 3,13");
			break;

		case 0x3D:
			sprintf(buffer,"LBI 3,14");
			break;

		case 0x3E:
			sprintf(buffer,"LBI 3,15");
			break;

		case 0x3F:
			sprintf(buffer,"LBI 3,0");
			break;

		// COMP
		case 0x40:
			sprintf(buffer,"COMP");
			break;

		//RMB 2
		case 0x42:
			sprintf(buffer,"RMB 2");
			break;

		//RMB 3
		case 0x43:
			sprintf(buffer,"RMB 3");
			break;

		//NOP
		case 0x44:
			sprintf(buffer,"NOP");
			break;

		//RMB 1
		case 0x45:
			sprintf(buffer,"RMB 1");
			break;

		//SMB 2
		case 0x46:
			sprintf(buffer,"SMB 2");
			break;

		//SMB 1
		case 0x47:
			sprintf(buffer,"SMB 1");
			break;

		//RET
		case 0x48:
			sprintf(buffer,"RET");
			break;

		//RETSK
		case 0x49:
			sprintf(buffer,"RETSK");
			break;

		//SMB 3
		case 0x4B:
			sprintf(buffer,"SMB 3");
			break;

		//RMB 0
		case 0x4C:
			sprintf(buffer,"RMB 0");
			break;

		//SMB 0
		case 0x4D:
			sprintf(buffer,"SMB 0");
			break;

		//CBA
		case 0x4E:
			sprintf(buffer,"CBA");
			break;

		//XAS
		case 0x4F:
			sprintf(buffer,"XAS");
			break;

		//CAB
			sprintf(buffer,"CAB");
			break;

		//ASIC
		case 0x51:
			sprintf(buffer,"AISC 1");
			break;

		case 0x52:
			sprintf(buffer,"AISC 2");
			break;

		case 0x53:
			sprintf(buffer,"AISC 3");
			break;

		case 0x54:
			sprintf(buffer,"AISC 4");
			break;

		case 0x55:
			sprintf(buffer,"AISC 5");
			break;

		case 0x56:
			sprintf(buffer,"AISC 6");
			break;

		case 0x57:
			sprintf(buffer,"AISC 7");
			break;

		case 0x58:
			sprintf(buffer,"AISC 8");
			break;

		case 0x59:
			sprintf(buffer,"AISC 9");
			break;

		case 0x5A:
			sprintf(buffer,"AISC 10");
			break;

		case 0x5B:
			sprintf(buffer,"AISC 11");
			break;

		case 0x5C:
			sprintf(buffer,"AISC 12");
			break;

		case 0x5D:
			sprintf(buffer,"AISC 13");
			break;

		case 0x5E:
			sprintf(buffer,"AISC 14");
			break;

		case 0x5F:
			sprintf(buffer,"AISC 15");
			break;

		case 0x60:
			addr =  cpu_readop(pc + 1);
			sprintf(buffer,"JMP %x",addr);
			cnt = 2;
			break;

		case 0x61:
			addr =  0x100 | cpu_readop(pc + 1);
			sprintf(buffer,"JMP %x",addr);
			cnt = 2;
			break;

		case 0x68:
			addr =  cpu_readop(pc + 1);
			sprintf(buffer,"JSR %x",addr);
			cnt = 2;
			break;

		case 0x69:
			addr =  0x100 | cpu_readop(pc + 1);
			sprintf(buffer,"JSR %x",addr);
			cnt = 2;
			break;

		//STII
		case 0x70:
			sprintf(buffer,"STII 0");
			break;

		case 0x71:
			sprintf(buffer,"STII 1");
			break;

		case 0x72:
			sprintf(buffer,"STII 2");
			break;

		case 0x73:
			sprintf(buffer,"STII 3");
			break;

		case 0x74:
			sprintf(buffer,"STII 4");
			break;

		case 0x75:
			sprintf(buffer,"STII 5");
			break;

		case 0x76:
			sprintf(buffer,"STII 6");
			break;

		case 0x77:
			sprintf(buffer,"STII 7");
			break;

		case 0x78:
			sprintf(buffer,"STII 8");
			break;

		case 0x79:
			sprintf(buffer,"STII 9");
			break;

		case 0x7A:
			sprintf(buffer,"STII 10");
			break;

		case 0x7B:
			sprintf(buffer,"STII 11");
			break;

		case 0x7C:
			sprintf(buffer,"STII 12");
			break;

		case 0x7D:
			sprintf(buffer,"STII 13");
			break;

		case 0x7E:
			sprintf(buffer,"STII 14");
			break;

		case 0x7F:
			sprintf(buffer,"STII 15");
			break;

		case 0xBF:
			sprintf(buffer,"LQID");
			break;

		case 0xFF:
			sprintf(buffer,"JID");
			break;

	}

	return cnt;
}
