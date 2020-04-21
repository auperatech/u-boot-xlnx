// raw-License-Identifier: raw from ethx-1.0+
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
#include <net.h>
#include "cJSON.h"

#define ETH_CMD_SIZE 256

#define CMD_ITEM	1

typedef struct {
	char cmd[ETH_CMD_SIZE];
} etn_cmd_list_t;

const static etn_cmd_list_t _cmd_list[CMD_ITEM] = {
	{"get-node-boot-info"}
};

#define AUP_RAW_PROTOCOL  0xEAE8
#define AUP_RAW_PKG_HEAD_LEN 17

#define ETH_DEBUG 0

uint8_t target_mac[6];
uint8_t source_mac[6];
int module_id;
int node_id;
#define AUP_REQ_METHOD			"get-node-boot-info"

extern int eth_mac_skip(int index);
extern void eth_parse_enetaddr(const char *addr, uint8_t *enetaddr);

static uchar net_pkt_buf[(PKTBUFSRX+1) * PKTSIZE_ALIGN + PKTALIGN];

void DeleteBlank (char *_str)
{
    int i,j;

	j=0;
    for(i=0;;i++)
    {
        if (_str[i]=='\0')
        {
            break;
        }

        if( (_str[i]!=' ')&&(_str[i]!='\t') )
        {
            _str[j]=_str[i];
            j++;
        }
    }
}

int GetcJSONArrayValue(cJSON * cJSON_Playload, char StrKey[], uint8_t * rArray, uint8_t size)
{
	int i=0;
	cJSON * cJSON_pArray = cJSON_GetObjectItem(cJSON_Playload, StrKey);
	cJSON *current_child = NULL;

	if ( cJSON_IsArray(cJSON_pArray) )
	{
		uint8_t index;

		index=cJSON_GetArraySize(cJSON_pArray);

		current_child = cJSON_pArray->child;
		i=0;
		while ( (current_child != NULL) && (i < index) && (i < size) )
		{
			rArray[i]=current_child->valueint;
			current_child = current_child->next;
			i++;
		}
	}

	return  i;
}


int GetNodeBootInfoHandler(char *PlayLoad,int argc,char * const argv[])
{
	char  ArrayStr[100];
	uint8_t ip[4];
	uint8_t mask[4];
	uint8_t gateway[4];
	uint8_t Dns1[4];
	uint8_t Dns2[4];
	uint8_t host[4];
	uint8_t ntp_server[4];
	unsigned int syslog_server_port=0;
	uint8_t syslog_server[4];
	uint8_t size=0;
	int     err=0;
	char    DlAddress[1500];
	int 	ret=-1;
	char    Dhcp_static[256];

	cJSON * cJSON_PlayLoad = cJSON_Parse(PlayLoad);
	if ( !cJSON_PlayLoad )
	{
		return ret;
	}

	memset(ArrayStr,0,sizeof(ArrayStr));
	memcpy(ArrayStr,"ip",strlen("ip"));
	size = sizeof(ip);
	if( size!=GetcJSONArrayValue(cJSON_PlayLoad,ArrayStr,ip,size) )
	{
		err++;
	}

	memset(DlAddress,0,sizeof(DlAddress));
	cJSON * cJSON_DLaddress = cJSON_GetObjectItem(cJSON_PlayLoad, "dl_address");
	if ( !cJSON_IsString(cJSON_DLaddress) )
	{
		err++;
	}
	else
	{
		memcpy(DlAddress,cJSON_DLaddress->valuestring,strlen(cJSON_DLaddress->valuestring));
		if(!strlen(DlAddress) )
		{
			memcpy(DlAddress,"x",strlen("x"));
		}
		else
		{
			DeleteBlank(DlAddress);
		}
	}

	memset(ArrayStr,0,sizeof(ArrayStr));
	memcpy(ArrayStr,"mask",strlen("mask"));
	size = sizeof(mask);
	if( size!=GetcJSONArrayValue(cJSON_PlayLoad,ArrayStr,mask,size) )
	{
		err++;
	}

	memset(ArrayStr,0,sizeof(ArrayStr));
	memcpy(ArrayStr,"gateway",strlen("gateway"));
	size = sizeof(gateway);
	if( size!=GetcJSONArrayValue(cJSON_PlayLoad,ArrayStr,gateway,size) )
	{
		err++;
	}


	memset(ArrayStr,0,sizeof(ArrayStr));
	memcpy(ArrayStr,"dns1",strlen("dns1"));
	size = sizeof(Dns1);
	if( size!=GetcJSONArrayValue(cJSON_PlayLoad,ArrayStr,Dns1,size) )
	{
		err++;
	}

	memset(ArrayStr,0,sizeof(ArrayStr));
	memcpy(ArrayStr,"dns2",strlen("dns2"));
	size = sizeof(Dns2);
	if( size!=GetcJSONArrayValue(cJSON_PlayLoad,ArrayStr,Dns2,size) )
	{
		err++;
	}


	memset(ArrayStr,0,sizeof(ArrayStr));
	memcpy(ArrayStr,"host",strlen("host"));
	size = sizeof(host);
	if( size!=GetcJSONArrayValue(cJSON_PlayLoad,ArrayStr,host,size) )
	{
		err++;
	}


	memset(ArrayStr,0,sizeof(ArrayStr));
	memcpy(ArrayStr,"ntp_server",strlen("ntp_server"));
	size = sizeof(ntp_server);
	if( size!=GetcJSONArrayValue(cJSON_PlayLoad,ArrayStr,ntp_server,size) )
	{
		err++;
	}


	memset(ArrayStr,0,sizeof(ArrayStr));
	memcpy(ArrayStr,"syslog_server",strlen("syslog_server"));
	size = sizeof(syslog_server);
	if( size!=GetcJSONArrayValue(cJSON_PlayLoad,ArrayStr,syslog_server,size) )
	{
		err++;
	}


	cJSON * cJSON_GetNumber = cJSON_GetObjectItem(cJSON_PlayLoad, "syslog_server_port");
	if ( !cJSON_IsNumber(cJSON_GetNumber) )
	{
		err++;
	}
	else
	{
		syslog_server_port=cJSON_GetNumber->valueint;
	}

	memset(Dhcp_static,0,sizeof(Dhcp_static));
	cJSON * cJSON_GetDhcpVal = cJSON_GetObjectItem(cJSON_PlayLoad, "use_dhcp");
	if ( !cJSON_IsNumber(cJSON_GetDhcpVal) )
	{
		memcpy(Dhcp_static,"static",strlen("static"));
	}
	else
	{
		if(cJSON_GetDhcpVal->valueint==1)
		{
			memcpy(Dhcp_static,"dhcp",strlen("dhcp"));
		}

		if(cJSON_GetDhcpVal->valueint==0)
		{
			memcpy(Dhcp_static,"static",strlen("static"));
		}
	}

	cJSON_Delete(cJSON_PlayLoad);

	if(!err)
	{
		ret = 0;
		printf("%d.%d.%d.%d ", ip[0], ip[1], ip[2], ip[3]);
		printf("%s ", DlAddress);
		printf("%d.%d.%d.%d ", mask[0], mask[1], mask[2], mask[3]);
		printf("%d.%d.%d.%d ", gateway[0], gateway[1], gateway[2], gateway[3]);
		printf("%d.%d.%d.%d ", Dns1[0], Dns1[1], Dns1[2], Dns1[3]);
		printf("%d.%d.%d.%d ", Dns2[0], Dns2[1], Dns2[2], Dns2[3]);
		printf("%d.%d.%d.%d ", host[0], host[1], host[2], host[3]);
		printf("%d.%d.%d.%d ", ntp_server[0], ntp_server[1], ntp_server[2], ntp_server[3]);
		printf("%d.%d.%d.%d ", syslog_server[0], syslog_server[1], syslog_server[2], syslog_server[3]);
		printf("%d", syslog_server_port);
		printf(" %s\n", Dhcp_static);

		if(argc)
		{
			memset(ArrayStr,0,sizeof(ArrayStr));
			snprintf(ArrayStr, sizeof(ArrayStr)-1, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
			env_set(argv[0], ArrayStr);
			printf("%s=%s\n",argv[0],ArrayStr);
			argc -= 1;
			argv += 1;
		}

		if(argc)
		{
			memset(ArrayStr,0,sizeof(ArrayStr));
			snprintf(ArrayStr, sizeof(ArrayStr)-1, "%s", DlAddress);
			env_set(argv[0], ArrayStr);
			printf("%s=%s\n",argv[0],ArrayStr);
			argc -= 1;
			argv += 1;
		}

		if(argc)
		{
			memset(ArrayStr,0,sizeof(ArrayStr));
			snprintf(ArrayStr, sizeof(ArrayStr)-1, "%d.%d.%d.%d", mask[0], mask[1], mask[2], mask[3]);
			env_set(argv[0], ArrayStr);
			printf("%s=%s\n",argv[0],ArrayStr);
			argc -= 1;
			argv += 1;
		}

		if(argc)
		{
			memset(ArrayStr,0,sizeof(ArrayStr));
			snprintf(ArrayStr, sizeof(ArrayStr)-1, "%d.%d.%d.%d", gateway[0], gateway[1], gateway[2], gateway[3]);
			env_set(argv[0], ArrayStr);
			printf("%s=%s\n",argv[0],ArrayStr);
			argc -= 1;
			argv += 1;
		}

		if(argc)
		{
			memset(ArrayStr,0,sizeof(ArrayStr));
			snprintf(ArrayStr, sizeof(ArrayStr)-1, "%d.%d.%d.%d", Dns1[0], Dns1[1], Dns1[2], Dns1[3]);
			env_set(argv[0], ArrayStr);
			printf("%s=%s\n",argv[0],ArrayStr);
			argc -= 1;
			argv += 1;
		}

		if(argc)
		{
			memset(ArrayStr,0,sizeof(ArrayStr));
			snprintf(ArrayStr, sizeof(ArrayStr)-1, "%d.%d.%d.%d", Dns2[0], Dns2[1], Dns2[2], Dns2[3]);
			env_set(argv[0], ArrayStr);
			printf("%s=%s\n",argv[0],ArrayStr);
			argc -= 1;
			argv += 1;
		}

		if(argc)
		{
			memset(ArrayStr,0,sizeof(ArrayStr));
			snprintf(ArrayStr, sizeof(ArrayStr)-1, "%d.%d.%d.%d", host[0], host[1], host[2], host[3]);
			env_set(argv[0], ArrayStr);
			printf("%s=%s\n",argv[0],ArrayStr);
			argc -= 1;
			argv += 1;
		}

		if(argc)
		{
			memset(ArrayStr,0,sizeof(ArrayStr));
			snprintf(ArrayStr, sizeof(ArrayStr)-1, "%d.%d.%d.%d", ntp_server[0], ntp_server[1], ntp_server[2], ntp_server[3]);
			env_set(argv[0], ArrayStr);
			printf("%s=%s\n",argv[0],ArrayStr);
			argc -= 1;
			argv += 1;
		}

		if(argc)
		{
			memset(ArrayStr,0,sizeof(ArrayStr));
			snprintf(ArrayStr, sizeof(ArrayStr)-1, "%d.%d.%d.%d", syslog_server[0], syslog_server[1], syslog_server[2], syslog_server[3]);
			env_set(argv[0], ArrayStr);
			printf("%s=%s\n",argv[0],ArrayStr);
			argc -= 1;
			argv += 1;
		}

		if(argc)
		{
			memset(ArrayStr,0,sizeof(ArrayStr));
			snprintf(ArrayStr, sizeof(ArrayStr)-1, "%d", syslog_server_port);
			env_set(argv[0], ArrayStr);
			printf("%s=%s\n",argv[0],ArrayStr);
			argc -= 1;
			argv += 1;
		}

		if(argc)
		{
			memset(ArrayStr,0,sizeof(ArrayStr));
			snprintf(ArrayStr, sizeof(ArrayStr)-1, "%s", Dhcp_static);
			env_set(argv[0], ArrayStr);
			printf("%s=%s\n",argv[0],ArrayStr);
			argc -= 1;
			argv += 1;
		}
	}

	return ret;
}

int32_t RawSocketVerifyPlayLoad(char * SrcPayLoad )
{
	if(!SrcPayLoad)
	{
		return -1;
	}

	cJSON * cJSON_PlayLoad = cJSON_Parse(SrcPayLoad);
	if ( !cJSON_PlayLoad )
	{
		return -1;
	}

	/* Check method */
	cJSON * cJSON_method = cJSON_GetObjectItem(cJSON_PlayLoad, "method");
	if ( !cJSON_IsString(cJSON_method) || strcmp(cJSON_method->valuestring, AUP_REQ_METHOD) )
	{
		cJSON_Delete(cJSON_PlayLoad);
		return -1;
	}

	cJSON * cJSON_GetModuleId = cJSON_GetObjectItem(cJSON_PlayLoad, "module_id");
	if ( !cJSON_IsNumber(cJSON_GetModuleId) )
	{
		cJSON_Delete(cJSON_PlayLoad);
		return -1;
	}
	else
	{
		if( (cJSON_GetModuleId->valueint)!= module_id )
		{
			cJSON_Delete(cJSON_PlayLoad);
			return -1;
		}

	}

	cJSON * cJSON_GetNodeId = cJSON_GetObjectItem(cJSON_PlayLoad, "node_id");
	if ( !cJSON_IsNumber(cJSON_GetNodeId) )
	{
		cJSON_Delete(cJSON_PlayLoad);
		return -1;
	}
	else
	{
		if( (cJSON_GetNodeId->valueint)!= node_id )
		{
			cJSON_Delete(cJSON_PlayLoad);
			return -1;
		}

	}

	cJSON_Delete(cJSON_PlayLoad);
	return 0;
}

int32_t IsHeader(uchar *ptr)
{
	if(!ptr)
	{
		return 0;
	}

	if ( ( ((AUP_RAW_PROTOCOL >> 8) & 0xff) == ptr[12] ) && ( (AUP_RAW_PROTOCOL & 0xff) == ptr[13] ) )
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

int32_t RawSocketVerifyPacket(uchar *p)
{
	if(!p)
	{
		return -1;
	}

	if (!IsHeader(p))
	{
		return -1;
	}

	for (int i = 0; i < 6; i++)
	{
		if (p[i] != source_mac[i])
		{
			return -1;
		}
	}

	for (int i = 0; i < 6; i++)
	{
		if (p[i+6] != target_mac[i])
		{
			return -1;
		}
	}
	return 0;
}

static int do_nodebootclient_bootinfo(cmd_tbl_t *cmdtp, int flag, int argc,
			     char * const argv[])
{
	struct udevice *dev = NULL;
	char raw_buffer[1536] = {0};
	char playload[1536] = {0};
	unsigned int playload_len = {0};
	unsigned char dest_mac[6];
	unsigned char src_mac[6];
	int rc;
	int ret = -EINVAL;
	uchar *packet;
	int i,j;
	unsigned int TimeOut=0;
	unsigned int VerifOK=0;
	struct eth_pdata *pdata;

	if ( argc < 5 )
	{
		return CMD_RET_USAGE;
	}

	module_id = (unsigned int)simple_strtol(argv[1], NULL, 10);
    node_id = (unsigned int)simple_strtol(argv[2], NULL, 10);

	rc=0;
	rc = sscanf(argv[3], "%hx:%hx:%hx:%hx:%hx:%hx", &dest_mac[0], &dest_mac[1],&dest_mac[2], &dest_mac[3], &dest_mac[4], &dest_mac[5]);
	if (rc!=6)
	{
		printf("mac is error.\n");
		return -1;
	}
	memcpy(target_mac,dest_mac,sizeof(dest_mac));

	rc=0;
	rc = sscanf(argv[4], "%hx:%hx:%hx:%hx:%hx:%hx", &src_mac[0], &src_mac[1],&src_mac[2], &src_mac[3], &src_mac[4], &src_mac[5]);
	if (rc!=6)
	{
		printf("mac is error.\n");
		return -1;
	}
	memcpy(source_mac,src_mac,sizeof(src_mac));

	memset(playload,0,sizeof(playload));
	memset(raw_buffer,0,sizeof(raw_buffer));

	snprintf(playload, sizeof(playload) -1, "{"
				"\"method\":\"%s\","
				"\"module_id\":%u,"
				"\"node_id\":%u"
				"}"
				, _cmd_list[0].cmd
				, module_id
				, node_id
				);

	/* Set Dest MAC address */
	raw_buffer[0] = dest_mac[0];
	raw_buffer[1] = dest_mac[1];
	raw_buffer[2] = dest_mac[2];
	raw_buffer[3] = dest_mac[3];
	raw_buffer[4] = dest_mac[4];
	raw_buffer[5] = dest_mac[5];

	/* Set src MAC address */
	raw_buffer[6] = src_mac[0];
	raw_buffer[7] = src_mac[1];
	raw_buffer[8] = src_mac[2];
	raw_buffer[9] = src_mac[3];
	raw_buffer[10] = src_mac[4];
	raw_buffer[11] = src_mac[5];

	/* Set Protocol */
	raw_buffer[12] = (AUP_RAW_PROTOCOL >> 8) & 0xff;
	raw_buffer[13] = AUP_RAW_PROTOCOL & 0xff;

	raw_buffer[16] = 0x0;

	playload_len = strlen(playload);

	/* Set playload length */
	raw_buffer[14] = playload_len >> 8;
	raw_buffer[15] = playload_len & 0xff;

	memcpy(&raw_buffer[AUP_RAW_PKG_HEAD_LEN] , playload, playload_len);

	dev = eth_get_dev();

	if (!dev)
	{
		printf("No ethernet found.\n");
		return -1;
	}
	else
	{
		printf("ethernet found: %s eth%d \n",dev->name,dev->seq);
	}

	pdata = dev_get_platdata(dev);
	/* clear the MAC address */
	memset(pdata->enetaddr, 0, ARP_HLEN);

	eth_parse_enetaddr(argv[4], pdata->enetaddr);
	if( eth_get_ops(dev)->write_hwaddr && !eth_mac_skip(dev->seq) )
	{
		if (!is_valid_ethaddr(pdata->enetaddr))
		{
			printf("\nError: %s eth%d address %pM illegal value\n",dev->name,dev->seq, pdata->enetaddr);
			return -EINVAL;
		}

		ret = eth_get_ops(dev)->write_hwaddr(dev);
		if (ret)
		{
			printf("\nWarning: %s eth%d failed to set MAC address\n",dev->name,dev->seq);
			return -EINVAL;
		}
	}

	eth_halt();

	eth_set_current();

	ret = eth_init();
	if (ret != 0)
	{
		printf("eth_init %s eth%d is fail\n",dev->name,dev->seq);
		return -EINVAL;
	}

	net_tx_packet = &net_pkt_buf[0] + (PKTALIGN - 1);
	net_tx_packet -= (ulong)net_tx_packet % PKTALIGN;

	memcpy(net_tx_packet,raw_buffer, AUP_RAW_PKG_HEAD_LEN+playload_len);

	TimeOut=1;
	VerifOK=0;
	for(i=0;i<15;i++)
	{
		if(TimeOut)
		{
			ret=eth_send((void*)net_tx_packet, AUP_RAW_PKG_HEAD_LEN+playload_len);
			if (ret < 0)
			{
				/* We cannot completely return the error at present */
				printf("%s: send() returned error %d\n", __func__, ret);
				eth_halt();
				return ret;
			}
			TimeOut=0;
			udelay(10000);
		}

		for (j = 0; j < 20; j++)
		{
			ret = eth_get_ops(dev)->recv(dev, ETH_RECV_CHECK_DEVICE, &packet);
			if (ret > 0)
			{
			#if ETH_DEBUG
				printf("[Recv] length=%d,\r\n\r\n",ret);

				printf("[Recv] Char start:\r\n\r\n");

				for (int k = 0; k < 17; ++k)
				{
					printf("%x ",packet[k] );
				}
				printf("\r\n");

				printf("%s\r\n",&packet[17]);

				printf("\r\n[Recv] Char END:\r\n\r\n\r\n\r\n");
			#endif

				if( !RawSocketVerifyPacket(packet) )
				{
					memset(playload,0,sizeof(playload));
					memcpy(playload,(char*)&packet[17],ret-17);
					if( !RawSocketVerifyPlayLoad(playload) )
					{
						argc -= 5;
						argv += 5;
						if( !GetNodeBootInfoHandler(playload,argc, argv) )
						{
							TimeOut=0;
							VerifOK=1;
						}
					}
				}

				if (eth_get_ops(dev)->free_pkt)
				{
					eth_get_ops(dev)->free_pkt(dev, packet, ret);
				}
			}

			if(VerifOK)
			{
				break;
			}
			udelay(5000);
		}

		if(VerifOK)
		{
			break;
		}
		else
		{
			TimeOut=1;
		}
	}

	eth_halt();

	return 0;
}


static cmd_tbl_t ethraw_commands[] = {
	U_BOOT_CMD_MKENT(nodebootinfo, 16, 1, do_nodebootclient_bootinfo, "", "")
};

static int do_nodebootclient(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	cmd_tbl_t *eth_cmd;
	int ret;

	if ( argc < 6 )
	{
		return CMD_RET_USAGE;
	}

	eth_cmd = find_cmd_tbl(argv[1], ethraw_commands,ARRAY_SIZE(ethraw_commands));

	argc -= 1;
	argv += 1;

	if ( !eth_cmd || argc > eth_cmd->maxargs )
	{
		return CMD_RET_USAGE;
	}

	ret = eth_cmd->cmd(eth_cmd, flag, argc, argv);

	return cmd_process_error(eth_cmd, ret);
}

#ifdef CONFIG_SYS_LONGHELP
static char nodebootclient_help_text[] =
	"command slot-id node-id dest-mac src-mac environment-variables(aup_nodeip,aup_dl_address,aup_mask,aup_gateway,aup_dns1,\n"
	"\taup_dns2,aup_hostip,aup_ntp_server,aup_syslog_server,aup_syslog_server_port)\n"
	"get-node-boot-info 27 0 88:e0:a0:20:73:fe 88:e0:a0:20:72:01 aup_nodeip aup_dl_address aup_mask aup_gateway aup_dns1\n"
	"\taup_dns2 aup_hostip aup_ntp_server aup_syslog_server aup_syslog_server_port";
#endif

U_BOOT_CMD(
	nodebootclient,	17,	1,	do_nodebootclient,
	"get node boot client info set environment variable from eth",
#ifdef CONFIG_SYS_LONGHELP
	nodebootclient_help_text
#endif
);

