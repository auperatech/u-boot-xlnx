// Uart-License-Identifier: UART-1.0+
/*
 * Copyright (c) 2019 Auperastor, Inc
 *
 * (C) Copyright 2019
 * Aupera Vasut <Auper@auperastor.com>
 */

#include <common.h>
#include <command.h>
#include <dm.h>
#include <malloc.h>
#include <mapmem.h>
#include <errno.h>
#include <asm/io.h>
#include <dm/root.h>
#include <dm/util.h>

#include <debug_uart.h>


#define CMD_ITEM	12
#define USART_CMD_SIZE 256
#define MCU_INFO_MAX   256
#define READ_MAX_CNT 5

typedef struct {
	char cmd[USART_CMD_SIZE];
} usart_cmd_list_t;

const static usart_cmd_list_t _cmd_list[CMD_ITEM] = {
	{"##get_boot_info##"},
	{"##get_network##"},
	{"##get_monitor_mac##"},
	{"##get_node_index##"},
	{"##set_node_link##"},
	{"^get_mcu_version$"},
	{"^get_firmware_version$"},
	{"^get_hardware_sn$"},
	{"^get_temporary_sn_info$"},
	{"^del_temporary_sn_info$"},
	{"^get_mcu_version_info$"},
	{"^get_running_state_info$"}
};

static int do_setenvuart_uartconf(cmd_tbl_t *cmdtp, int flag, int argc,
			  char * const argv[])
{
	unsigned int portnum;
	unsigned int baudrate;

	if ( argc < 3 )
	{
		return CMD_RET_USAGE;
	}

    portnum = (unsigned int)simple_strtol(argv[1], NULL, 0);
    baudrate = (unsigned int)simple_strtol(argv[2], NULL, 0);

	if(!uartx_config(portnum,baudrate))
	{
		//printf("Config uart%d %d\r\n",portnum,baudrate);
	}

	return 0;
}

static int do_setenvuart_bootinfo(cmd_tbl_t *cmdtp, int flag, int argc,
			  char * const argv[])
{
	char CpuMAC[MCU_INFO_MAX];
	char ManageMAC[MCU_INFO_MAX];
	char SlotID[MCU_INFO_MAX];
	char NodeID[MCU_INFO_MAX];
	char ChassType[MCU_INFO_MAX];
	char target[MCU_INFO_MAX];
	char buffer[MCU_INFO_MAX];
	char buffer1[MCU_INFO_MAX];
	int readsize=0;
   	int i, j,k;
	int HeadChar=0;
	int EndChar=0;
	int rc =0;
	unsigned int portnum;
	unsigned int ModuleSlotID;
	unsigned int ModuleNodeID;
	unsigned int ModuleChassType;
	unsigned int bootinfoCRC=0;
	unsigned int CrcPost=0;

	if ( argc < 6 )
	{
		return CMD_RET_USAGE;
	}

	portnum = (unsigned int)simple_strtol(argv[1], NULL, 0);

	for(k=0;k<READ_MAX_CNT;k++)
	{
		uartx_write(portnum,_cmd_list[0].cmd,strlen(_cmd_list[0].cmd));
		memset(buffer, 0, sizeof(buffer));
		readsize=uartx_read(portnum,buffer,sizeof(buffer)-1);

		//printf("bootinfo:%s\r\n",buffer);

		if(!readsize)
			continue;

		HeadChar=0;EndChar=0;
		memset(target, 0, sizeof(target));
	    for (i = 0, j = 0; i < strlen(buffer); i++)
	    {
	        if( buffer[i] == '@' )
	        {
	        	if(HeadChar)
	        	{
					j=0;
					memset(target, 0, sizeof(target));
	        	}
				HeadChar=1;
	        }

	        if( HeadChar )
	        {
	        	target[j++] = buffer[i];

		        if( i==strlen(buffer)-1 )
		        {
		        	if( buffer[i] == '#' )
		        	{
			        	EndChar = 1;
			        	break;
			        }
		        }
	       	}
	    }

		if( HeadChar && EndChar )
		{
			break;
		}
	}

	if( HeadChar && EndChar )//"@#%s&#%s$#%02d*#%02d!#%02d#"
	{
		memset(CpuMAC,0,sizeof(CpuMAC));
		memset(ManageMAC,0,sizeof(ManageMAC));
		memset(SlotID,0,sizeof(SlotID));
		memset(NodeID,0,sizeof(NodeID));
		memset(ChassType,0,sizeof(ChassType));

		CrcPost=2;
		for(i=0;i<strlen(target)-CrcPost;i++)
		{
			if( ( (target[CrcPost+i]>='0')&&(target[CrcPost+i]<='9') ) ||
			    ( (target[CrcPost+i]>='a')&&(target[CrcPost+i]<='f') ) ||
			    ( (target[CrcPost+i]>='A')&&(target[CrcPost+i]<='F') )
			  )
			{
				CpuMAC[i]=target[CrcPost+i];
			}
			else
			{
				int len=strlen(CpuMAC);

				if( !len || (len%2) )
				{
					bootinfoCRC++;
				}
				CrcPost=CrcPost+i+2;
				break;
			}
		}

		for(i=0;i<strlen(target)-CrcPost;i++)
		{
			if( ( (target[CrcPost+i]>='0')&&(target[CrcPost+i]<='9') ) ||
			    ( (target[CrcPost+i]>='a')&&(target[CrcPost+i]<='f') ) ||
			    ( (target[CrcPost+i]>='A')&&(target[CrcPost+i]<='F') )
			  )
			{
				ManageMAC[i]=target[CrcPost+i];
			}
			else
			{
				int len=strlen(ManageMAC);

				if( !len || (len%2) )
				{
					bootinfoCRC++;
				}
				CrcPost=CrcPost+i+2;
				break;
			}
		}

		for(i=0,j=0;i<strlen(target)-CrcPost;i++)
		{
			if( ( (target[CrcPost+i]>='0')&&(target[CrcPost+i]<='9') ) ||
			    ( (target[CrcPost+i]>='a')&&(target[CrcPost+i]<='f') ) ||
			    ( (target[CrcPost+i]>='A')&&(target[CrcPost+i]<='F') )
			  )
			{
				SlotID[j++]=target[CrcPost+i];
			}
			else
			{
				int len=strlen(SlotID);

				if( !len )
				{
					bootinfoCRC++;
				}
				else
				{
					rc = sscanf(SlotID,"%d",&ModuleSlotID);
					if(rc==1)
					{
						memset(SlotID,0,sizeof(SlotID));
						snprintf(SlotID, MCU_INFO_MAX-1, "%d",ModuleSlotID);
					}
					else
					{
						bootinfoCRC++;
					}
				}
				CrcPost=CrcPost+i+2;
				break;
			}
		}

		for(i=0;i<strlen(target)-CrcPost;i++)
		{
			if( ( (target[CrcPost+i]>='0')&&(target[CrcPost+i]<='9') ) ||
			    ( (target[CrcPost+i]>='a')&&(target[CrcPost+i]<='f') ) ||
			    ( (target[CrcPost+i]>='A')&&(target[CrcPost+i]<='F') )
			  )
			{
				NodeID[i]=target[CrcPost+i];
			}
			else
			{
				int len=strlen(NodeID);

				if( !len )
				{
					bootinfoCRC++;
				}
				else
				{
					rc = sscanf(NodeID,"%d",&ModuleNodeID);
					if(rc==1)
					{
						memset(NodeID,0,sizeof(NodeID));
						snprintf(NodeID, MCU_INFO_MAX-1, "%d",ModuleNodeID);
					}
					else
					{
						bootinfoCRC++;
					}
				}
				CrcPost=CrcPost+i+2;
				break;
			}
		}

		for(i=0;i<strlen(target)-CrcPost;i++)
		{
			if( ( (target[CrcPost+i]>='0')&&(target[CrcPost+i]<='9') ) ||
			    ( (target[CrcPost+i]>='a')&&(target[CrcPost+i]<='f') ) ||
			    ( (target[CrcPost+i]>='A')&&(target[CrcPost+i]<='F') )
			  )
			{
				ChassType[i]=target[CrcPost+i];
			}
			else
			{
				int len=strlen(ChassType);

				if( !len )
				{
					bootinfoCRC++;
				}
				else
				{
					rc = sscanf(ChassType,"%d",&ModuleChassType);
					if(rc==1)
					{
						memset(ChassType,0,sizeof(ChassType));
						snprintf(ChassType, MCU_INFO_MAX-1, "%d",ModuleChassType);
					}
					else
					{
						bootinfoCRC++;
					}
				}
				break;
			}
		}

		if(!bootinfoCRC)
		{
			memset(buffer, 0, sizeof(buffer));
			memset(buffer1, 0, sizeof(buffer1));
			memcpy(buffer,CpuMAC,strlen(CpuMAC));
			memcpy(buffer1,ManageMAC,strlen(ManageMAC));
			memset(CpuMAC,0,sizeof(CpuMAC));
			memset(ManageMAC,0,sizeof(ManageMAC));

			for(i=0,j=0;i<strlen(buffer);i++)
			{
				if( i && !(i%2) )
				{
					CpuMAC[j++]=':';
				}

				CpuMAC[j++]=buffer[i];
			}

			for(i=0,j=0;i<strlen(buffer1);i++)
			{
				if( i && !(i%2) )
				{
					ManageMAC[j++]=':';
				}

				ManageMAC[j++]=buffer1[i];
			}
		}
	}
	else
	{
		bootinfoCRC++;
	}

	if(!bootinfoCRC)
	{
		printf("%s=%s\r\n",argv[2],CpuMAC);
		printf("%s=%s\r\n",argv[3],ManageMAC);
		printf("%s=%s\r\n",argv[4],SlotID);
		printf("%s=%s\r\n",argv[5],NodeID);
		printf("%s=%s\r\n",argv[6],ChassType);

		env_set(argv[2], CpuMAC);
		env_set(argv[3], ManageMAC);
		env_set(argv[4], SlotID);
		env_set(argv[5], NodeID);
		env_set(argv[6], ChassType);
	}

	return 0;
}

static int do_setenvuart_moduletype(cmd_tbl_t *cmdtp, int flag, int argc,
			     char * const argv[])
{
	char ModuleVer[MCU_INFO_MAX];
	char target[MCU_INFO_MAX];
	char buffer[MCU_INFO_MAX];
	int readsize=0;
   	int i, j,k;
	int HeadChar=0;
	int EndChar=0;
	int rcB =0,rcA =0;
	int Mode_Type=0,Mode_Version=0;
	unsigned int portnum;
	unsigned int VerOK=0;

	if ( argc < 2 )
	{
		return CMD_RET_USAGE;
	}

	memset(ModuleVer,0,sizeof(ModuleVer));
	portnum = (unsigned int)simple_strtol(argv[1], NULL, 0);

	for(k=0;k<READ_MAX_CNT;k++)
	{
		uartx_write(portnum,_cmd_list[10].cmd,strlen(_cmd_list[10].cmd));
		memset(buffer, 0, sizeof(buffer));
		readsize=uartx_read(portnum,buffer,sizeof(buffer)-1);

		//printf("MCU_VERSION:%s\r\n",buffer);

		if(!readsize)
			continue;

		HeadChar=0;EndChar=0;
		memset(target, 0, sizeof(target));
	    for (i = 0, j = 0; i < MCU_INFO_MAX; i++)
	    {
	        if( buffer[i] == '^' )
	        {
	        	if(HeadChar)
	        	{
					j=0;
					memset(target, 0, sizeof(target));
	        	}
				HeadChar=1;
	            continue;
	        }

	        if( buffer[i] == '$' )
	        {
	        	if(HeadChar)
	        	{
		        	EndChar = 1;
		        	break;
		        }
	        }

	        if( HeadChar )
	        {
	        	target[j++] = buffer[i];
	       	}
	    }

		if( !HeadChar || !EndChar )
		{
			continue;
		}

		Mode_Type=0;
		Mode_Version=0;
		rcB = sscanf(target, "Type:V205.%d.B%d,Ver",&Mode_Type,&Mode_Version);

		if(rcB!=2)
		{
			Mode_Type=0;
			Mode_Version=0;
			rcA = sscanf(target, "Type:V205.%d.A%d,Ver",&Mode_Type,&Mode_Version);
		}

		if( (rcB==2) )
		{
			if(Mode_Type==2) //V205
			{
				snprintf(ModuleVer, MCU_INFO_MAX-1, "V205B%d",Mode_Version);
				VerOK=1;
				break;
			}
		}

		if( (rcA==2) )
		{
			if(Mode_Type==2) //V205
			{
				snprintf(ModuleVer, MCU_INFO_MAX-1, "V205A%d",Mode_Version);
				VerOK=1;
				break;
			}
		}

	}

	if( VerOK )
	{
		env_set(argv[2], ModuleVer);
	}
	else
	{
		return -1;
	}

	return 0;
}

static int do_setenvuart_mcuversion(cmd_tbl_t *cmdtp, int flag, int argc,
			     char * const argv[])
{
	char McuVer[MCU_INFO_MAX];
	char target[MCU_INFO_MAX];
	char buffer[MCU_INFO_MAX];
	int readsize=0;
	int i, j,k;
	int HeadChar=0;
	int EndChar=0;
	unsigned int portnum;

	if ( argc < 2 )
	{
		return CMD_RET_USAGE;
	}

	memset(McuVer,0,sizeof(McuVer));
	portnum = (unsigned int)simple_strtol(argv[1], NULL, 0);

	for(k=0;k<READ_MAX_CNT;k++)
	{
		uartx_write(portnum,_cmd_list[10].cmd,strlen(_cmd_list[10].cmd));
		memset(buffer, 0, sizeof(buffer));
		readsize=uartx_read(portnum,buffer,sizeof(buffer)-1);

		//printf("MCU_VERSION:%s\r\n",buffer);

		if(!readsize)
			continue;

		HeadChar=0;EndChar=0;
		memset(target, 0, sizeof(target));
	    for (i = 0, j = 0; i < MCU_INFO_MAX; i++)
	    {
	        if( buffer[i] == '^' )
	        {
	        	if(HeadChar)
	        	{
					j=0;
					memset(target, 0, sizeof(target));
	        	}
				HeadChar=1;
	            continue;
	        }

	        if( buffer[i] == '$' )
	        {
	        	if(HeadChar)
	        	{
		        	EndChar = 1;
		        	break;
		        }
	        }

	        if( HeadChar )
	        {
	        	target[j++] = buffer[i];
	       	}
	    }

		if( HeadChar && EndChar )
		{
			snprintf(McuVer, MCU_INFO_MAX-1, "%s",target);
			break;
		}
	}

	if( !HeadChar || !EndChar )
	{
		return -1;
	}

	env_set(argv[2], McuVer);

	return 0;
}


static int do_setenvuart_runningstate(cmd_tbl_t *cmdtp, int flag, int argc,
			     char * const argv[])
{
	char RunState[MCU_INFO_MAX];
	char target[MCU_INFO_MAX];
	char buffer[MCU_INFO_MAX];
	int readsize=0;
   	int i, j,k;
	int HeadChar=0;
	int EndChar=0;
	unsigned int portnum;

	if ( argc < 2 )
	{
		return CMD_RET_USAGE;
	}

	memset(RunState,0,sizeof(RunState));
	portnum = (unsigned int)simple_strtol(argv[1], NULL, 0);

	for(k=0;k<READ_MAX_CNT;k++)
	{
		uartx_write(portnum,_cmd_list[11].cmd,strlen(_cmd_list[11].cmd));
		memset(buffer, 0, sizeof(buffer));
		readsize=uartx_read(portnum,buffer,sizeof(buffer)-1);

		//printf("Runing State:%s\r\n",buffer);

		if(!readsize)
			continue;

		HeadChar=0;EndChar=0;
		memset(target, 0, sizeof(target));
	    for (i = 0, j = 0; i < MCU_INFO_MAX; i++)
	    {
	        if( buffer[i] == '^' )
	        {
	        	if(HeadChar)
	        	{
					j=0;
					memset(target, 0, sizeof(target));
	        	}
				HeadChar=1;
	            continue;
	        }

	        if( buffer[i] == '$' )
	        {
	        	if(HeadChar)
	        	{
		        	EndChar = 1;
		        	break;
		        }
	        }

	        if( HeadChar )
	        {
	        	target[j++] = buffer[i];
	       	}
	    }

		if( HeadChar && EndChar )
		{
			snprintf(RunState, MCU_INFO_MAX-1, "%s",target);
			break;
		}
	}

	if( !HeadChar || !EndChar )
	{
		return -1;
	}

	env_set(argv[2], RunState);

	return 0;
}

static int do_setenvuart_read(cmd_tbl_t *cmdtp, int flag, int argc,
			     char * const argv[])
{
	unsigned int portnum;
	unsigned int *addr=0;
	unsigned int ReadSize=0;

	if ( argc < 4 )
	{
		return CMD_RET_USAGE;
	}

	portnum = (unsigned int)simple_strtol(argv[1], NULL, 0);
	ReadSize = (unsigned int)simple_strtol(argv[2], NULL, 0);
	addr = (unsigned int *)simple_strtol(argv[3], NULL, 16);

    if ( !addr )
    {
        return CMD_RET_USAGE;
    }

	memset((char *)addr,0,ReadSize);
	uartx_read(portnum,(char *)addr,ReadSize);

	return 0;
}

static int do_setenvuart_write(cmd_tbl_t *cmdtp, int flag, int argc,
			     char * const argv[])
{
	unsigned int portnum;
	unsigned int WriteSize=0;

	if ( argc < 3 )
	{
		return CMD_RET_USAGE;
	}

	portnum = (unsigned int)simple_strtol(argv[1], NULL, 0);

	WriteSize=strlen(argv[2]);

	uartx_write(portnum,argv[2],WriteSize);

	return 0;
}

static cmd_tbl_t uart_commands[] = {
	U_BOOT_CMD_MKENT(uartconf, 3, 1, do_setenvuart_uartconf, "", ""),
	U_BOOT_CMD_MKENT(bootinfo, 7, 1, do_setenvuart_bootinfo, "", ""),
	U_BOOT_CMD_MKENT(moduletype, 3, 1, do_setenvuart_moduletype, "", ""),
	U_BOOT_CMD_MKENT(mcuversion, 3, 1, do_setenvuart_mcuversion, "", ""),
	U_BOOT_CMD_MKENT(runningstate, 3, 1, do_setenvuart_runningstate, "", ""),
	U_BOOT_CMD_MKENT(read, 4, 1, do_setenvuart_read, "", ""),
	U_BOOT_CMD_MKENT(write, 3, 1, do_setenvuart_write, "", ""),
};

static int do_setenvuart(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	cmd_tbl_t *uart_cmd;
	int ret;

	if ( argc < 3 )
	{
		return CMD_RET_USAGE;
	}

	uart_cmd = find_cmd_tbl(argv[1], uart_commands,ARRAY_SIZE(uart_commands));

	argc -= 1;
	argv += 1;

	if ( !uart_cmd || argc > uart_cmd->maxargs )
	{
		return CMD_RET_USAGE;
	}

	ret = uart_cmd->cmd(uart_cmd, flag, argc, argv);

	return cmd_process_error(uart_cmd, ret);
}

#ifdef CONFIG_SYS_LONGHELP
static char setenvuart_help_text[] =
	"name environment-variable\n"
	"uartconf      -Set uart port baudrate\n"
	"bootinfo      -Set command uart-port CpuMAC ManageMAC SlotID NodeID ChassType \n"
	"moduletype    -Set command uart-port Aup Module Type \n"
	"mcuversion    -Set command uart-port MCU Version \n"
	"runningstate  -Set command uart-port Running sate \n"
	"read          -Set command uart-port Read to memory \n"
	"write         -Set command uart-port Write command \n"
	"- example: setenvuart uartconf 1 115200;\n"
    "- example: setenvuart bootinfo 1 CpuMAC ManageMAC SlotID NodeID ChassType;\n"
	"- example: setenvuart moduletype 1 V205_Type;\n"
	"- example: setenvuart mcuversion 1 McuVersion;\n"
	"- example: setenvuart runningstate 1 RuningState;\n"
	"- example: setenvuart read 1 ReadSize MemAddr;\n"
	"- example: setenvuart write 1 command;\n";
#endif

U_BOOT_CMD(
	setenvuart,	8,	1,	do_setenvuart,
	"set environment variable from uart",
#ifdef CONFIG_SYS_LONGHELP
	setenvuart_help_text
#endif
);
