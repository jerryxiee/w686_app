#include "Usr_main.h"

#define FILE_OFFSET_IP_ADDR         20
#define FILE_OFFSET_IP_SIZE         24
#define FILE_OFFSET_FW_ADDR         28
#define FILE_OFFSET_FW_SIZE         32

#define UART_SLIP_SIZE_MAX		    128

#define	SLIP_END				    0xC0
#define	SLIP_ESC				    0xDB
#define	SLIP_ESC_END			    0xDC
#define	SLIP_ESC_ESC			    0xDD

#define MIN(a,b) (((a) < (b)) ? (a) : (b))


//数据包类型
typedef enum
{
	NRF_DFU_OP_PROTOCOL_VERSION  = 0x00,     //!< Retrieve protocol version.
	NRF_DFU_OP_OBJECT_CREATE     = 0x01,     //!< Create selected object.
	NRF_DFU_OP_RECEIPT_NOTIF_SET = 0x02,     //!< Set receipt notification.
	NRF_DFU_OP_CRC_GET           = 0x03,     //!< Request CRC of selected object.
	NRF_DFU_OP_OBJECT_EXECUTE    = 0x04,     //!< Execute selected object.
	NRF_DFU_OP_OBJECT_SELECT     = 0x06,     //!< Select object.
	NRF_DFU_OP_MTU_GET           = 0x07,     //!< Retrieve MTU size.
	NRF_DFU_OP_OBJECT_WRITE      = 0x08,     //!< Write selected object.
	NRF_DFU_OP_PING              = 0x09,     //!< Ping.
	NRF_DFU_OP_HARDWARE_VERSION  = 0x0A,     //!< Retrieve hardware version.
	NRF_DFU_OP_FIRMWARE_VERSION  = 0x0B,     //!< Retrieve firmware version.
	NRF_DFU_OP_ABORT             = 0x0C,     //!< Abort the DFU procedure.
	NRF_DFU_OP_RESPONSE          = 0x60,     //!< Response.
	NRF_DFU_OP_INVALID           = 0xFF
} Bt_DfuDtata_Type;

//执行结果
typedef enum
{
	NRF_DFU_RES_CODE_INVALID                 = 0x00,    //!< Invalid opcode.
	NRF_DFU_RES_CODE_SUCCESS                 = 0x01,    //!< Operation successful.
	NRF_DFU_RES_CODE_OP_CODE_NOT_SUPPORTED   = 0x02,    //!< Opcode not supported.
	NRF_DFU_RES_CODE_INVALID_PARAMETER       = 0x03,    //!< Missing or invalid parameter value.
	NRF_DFU_RES_CODE_INSUFFICIENT_RESOURCES  = 0x04,    //!< Not enough memory for the data object.
	NRF_DFU_RES_CODE_INVALID_OBJECT          = 0x05,    //!< Data object does not match the firmware and hardware requirements, the signature is wrong, or parsing the command failed.
	NRF_DFU_RES_CODE_UNSUPPORTED_TYPE        = 0x07,    //!< Not a valid object type for a Create request.
	NRF_DFU_RES_CODE_OPERATION_NOT_PERMITTED = 0x08,    //!< The state of the DFU process does not allow this operation.
	NRF_DFU_RES_CODE_OPERATION_FAILED        = 0x0A,    //!< Operation failed.
	NRF_DFU_RES_CODE_EXT_ERROR               = 0x0B,    //!< Extended error. The next byte of the response contains the error code of the extended error (see @ref nrf_dfu_ext_error_code_t.
} nrf_dfu_result_t;


/**
* @brief @ref NRF_DFU_OP_OBJECT_SELECT response details.
*/
typedef struct
{
	u32 offset;                    //!< Current offset.
	u32 crc;                       //!< Current CRC.
	u32 max_size;                  //!< Maximum size of selected object.
} nrf_dfu_response_select_t;

typedef struct
{
	u32 offset;                    //!< Current offset.
	u32 crc;                       //!< Current CRC.
} nrf_dfu_response_crc_t;


BTDFU_INFO  BtDfuInfo;
char BtDfu_SendBuf[150];            //蓝牙发送数据转译后的结果
char Bt_SendBuf[150];               //蓝牙发送数据buf
u8 	 BtDfu_RevBuf[150];             //蓝牙接收数据转译后的结果
u16  BtDfu_RevBuf_len;              //蓝牙转译之后的数据长度

static u8  ping_id = 0;
static u16 prn = 0;
static u16 mtu = 0;


int dfu_drv_tx(const u8 *p_data, u16 length);
int dfu_host_send_ip(u8 *p_data, u32 data_size);
int dfu_host_send_fw(const u8* p_data, u32 data_size);
static int stream_data_crc(const u8* p_data, u32 data_size, u32 pos, u32* p_crc);
static int req_obj_select(u8 obj_type, nrf_dfu_response_select_t* p_select_rsp);
static int req_obj_execute(void);
static int req_get_crc(nrf_dfu_response_crc_t* p_crc_rsp);
static int req_obj_create(u8 obj_type, u32 obj_size);

static u16 get_u16_le(const u8* p_data)
{
	u16 data;

	data  = ((u16)*(p_data + 0) << 0);
	data += ((u16)*(p_data + 1) << 8);

	return data;
}

static void put_u16_le(u8* p_data, u16 data)
{
	*(p_data + 0) = (u8)(data >> 0);
	*(p_data + 1) = (u8)(data >> 8);
}

static u32 get_u32_le(const u8* p_data)
{
	u32 data;

	data  = ((u32)*(p_data + 0) <<  0);
	data += ((u32)*(p_data + 1) <<  8);
	data += ((u32)*(p_data + 2) << 16);
	data += ((u32)*(p_data + 3) << 24);

	return data;
}

static void put_u32_le(u8* p_data, u32 data)
{
	*(p_data + 0) = (u8)(data >>  0);
	*(p_data + 1) = (u8)(data >>  8);
	*(p_data + 2) = (u8)(data >> 16);
	*(p_data + 3) = (u8)(data >> 24);
}

u32 crc32_compute(uint8_t const * p_data, u32 size, u32 const * p_crc)
{
    u32 crc;
    u32 i, j;

    crc = (p_crc == NULL) ? 0xFFFFFFFF : ~(*p_crc);     //如果p_crc有值就使用p_crc取反给crc，否则就用0xFFFFFFFF

    for (i = 0; i < size; i++)
    {
        crc = crc ^ p_data[i];
        for (j = 8; j > 0; j--)
        {
            crc = (crc >> 1) ^ (0xEDB88320U & ((crc & 1) ? 0xFFFFFFFF : 0));
        }
    }
    return ~crc;
}


static int send_raw(const u8* pData, u32 nSize)
{
	return dfu_drv_tx(pData, nSize);
}

//pSrcData原buf数据；nSrcSize，原数据长度；pDestData，转译后的buf数据；pDestSize转译后的数据长度
void encode_slip(u8 *pDestData, u32 *pDestSize, const u8 *pSrcData, u32 nSrcSize)
{
	u32 n, nDestSize;

	nDestSize = 0;

	for (n = 0; n < nSrcSize; n++)
	{
		u8 nSrcByte = *(pSrcData + n);

		if (nSrcByte == SLIP_END)
		{
			*pDestData++ = SLIP_ESC;
			*pDestData++ = SLIP_ESC_END;
			nDestSize += 2;
		}
		else if (nSrcByte == SLIP_ESC)
		{
			*pDestData++ = SLIP_ESC;
			*pDestData++ = SLIP_ESC_ESC;
			nDestSize += 2;
		}
		else
		{
			*pDestData++ = nSrcByte;
			nDestSize++;
		}
	}

	*pDestData = SLIP_END;
	nDestSize++;

	*pDestSize = nDestSize;
}

int decode_slip(u8 *pDestData, u32 *pDestSize, const u8 *pSrcData, u32 nSrcSize)
{
	int err_code = 1;
	u32 n, nDestSize = 0;
	bool is_escaped = false;

	for (n = 0; n < nSrcSize; n++)
	{
		u8 nSrcByte = *(pSrcData + n);

		if (nSrcByte == SLIP_END)
		{
			if (!is_escaped)
				err_code = 0;  // Done. OK

			break;
		}
		else if (nSrcByte == SLIP_ESC)
		{
			if (is_escaped)
			{
				// should not get SLIP_ESC twice...
				err_code = 1;
				break;
			}
			else
				is_escaped = true;
		}
		else if (nSrcByte == SLIP_ESC_END)
		{
			if (is_escaped)
			{
				is_escaped = false;

				*pDestData++ = SLIP_END;
			}
			else
				*pDestData++ = nSrcByte;

			nDestSize++;
		}
		else if (nSrcByte == SLIP_ESC_ESC)
		{
			if (is_escaped)
			{
				is_escaped = false;

				*pDestData++ = SLIP_ESC;
			}
			else
				*pDestData++ = nSrcByte;

			nDestSize++;
		}
		else
		{
			*pDestData++ = nSrcByte;
			nDestSize++;
		}
	}

	*pDestSize = nDestSize;

	return err_code;
}

//转译数据并发送
int dfu_drv_tx(const u8 *p_data, u16 length)
{
	int rc = 0;
	u32 slip_pkt_len;

	if (length > UART_SLIP_SIZE_MAX) {
		rc = -3;
	}
	else {
        memset(BtDfu_SendBuf,0,sizeof(BtDfu_SendBuf));
		encode_slip((u8 *)BtDfu_SendBuf, &slip_pkt_len, p_data, length);		//转译关键字0x0c

        UART_Send(USART4, (u8 *)BtDfu_SendBuf,(u16)slip_pkt_len);
	}

	return rc;
}


static int try_recover_ip(const u8* p_data, u32 data_size,
						  nrf_dfu_response_select_t* p_rsp_recover,
						  const nrf_dfu_response_select_t* p_rsp_select)
{
	printf("%s\r\n", __func__);	

	int rc = 0;
	u32 pos_start, len_remain;
	u32 crc_32;

	*p_rsp_recover = *p_rsp_select;

	pos_start = p_rsp_recover->offset;

	if (pos_start > 0 && pos_start <= data_size)
	{
		crc_32 = crc32_compute(p_data, pos_start, NULL);

		if (p_rsp_select->crc != crc_32)
		{
			pos_start = 0;
		}
	}
	else
	{
		pos_start = 0;
	}

	if (pos_start > 0 && pos_start < data_size)
	{
		len_remain = data_size - pos_start;
		rc = stream_data_crc(p_data + pos_start, len_remain, pos_start, &crc_32);
		if (!rc)
		{
			pos_start += len_remain;
		}
		else if (rc == 2)
		{
			// when there is a CRC error, discard previous init packet
			rc = 0;
			pos_start = 0;
		}
	}

	if (!rc && pos_start == data_size)
	{
		rc = req_obj_execute();
	}

	p_rsp_recover->offset = pos_start;

	return rc;
}


//读取bin文件中开头的info部分内容，该内容存放了文件IP地址，IP大小，固件地址和固件大小信息
void dfu_file_info(u32* ip_addr, u32* ip_size, u32* fw_addr, u32* fw_size)
{
    u8 p_file_header[100] = {0};

    EXFLASH_ReadBuffer(p_file_header,EXFLASH_BTAPP_ADDR,100);

    *ip_addr = 0x00088080;
    *fw_addr = 0x00088280;

    *ip_size = get_u32_le(&p_file_header[FILE_OFFSET_IP_SIZE]);
    *fw_size = get_u32_le(&p_file_header[FILE_OFFSET_FW_SIZE]);

    printf("ip addr: %08x\r\n", *ip_addr);
    printf("ip size: %08x\r\n", *ip_size);
    printf("fw addr: %08x\r\n", *fw_addr);
    printf("fw size: %08x\r\n", *fw_size);
}

//阻塞接收串口数据
int dfu_drv_rx(u8 *p_data, u32 max_len, u32 *p_real_len)
{
	int rc = 0;
//	u32 start_time;

    BtDfuInfo.WaitRspDataOverTime = 20;
	while (true) 
    {
		if (BtDfuInfo.OnePackRecvOver == 1) 
		{
			decode_slip(p_data, p_real_len, (u8 *)BtDfu_RevBuf, BtDfu_RevBuf_len);

			if (*p_real_len > max_len) {
				rc = -2;
			}

			BtDfuInfo.OnePackRecvOver = 0;

			break;
		}

        if(BtDfuInfo.WaitRspDataOverTime == 0)
        {
            rc = -1;
            break;
        }
	}
	return rc;
}

//接收数据，并判断数据的合法性
static int get_rsp(Bt_DfuDtata_Type oper, u32* p_data_cnt)
{
	printf("%s\r\n", __func__);

	int rc;


	rc = dfu_drv_rx((u8 *)Uart4Buf, sizeof(Uart4Buf), p_data_cnt);		//读取数据

	if (!rc)
	{
		if (*p_data_cnt >= 3 &&												//判断数据接收的有效长度
			BtDfu_RevBuf[0] == NRF_DFU_OP_RESPONSE &&						    //判断是否为0x60开头
			BtDfu_RevBuf[1] == oper)										    //判断接收消息类型是否与当前发送的一致
		{
			if (BtDfu_RevBuf[2] != NRF_DFU_RES_CODE_SUCCESS)				    //如果执行结果出错
			{
				u16 rsp_error = BtDfu_RevBuf[2];

				// get 2-byte error code, if applicable
				if (*p_data_cnt >= 4)
					rsp_error = (rsp_error << 8) + BtDfu_RevBuf[3];

				printf("\r\nBad result code (0x%X)!", rsp_error);				//打印出错结果

				rc = 1;
			}
		}
		else
		{
			printf("\r\nInvalid response!");

			rc = 1;
		}
	}

	return rc;
}

//发送ping数据
static int req_ping(u8 id)
{
	printf("%s\r\n", __func__);	

	int rc;
	u8 send_data[2] = { NRF_DFU_OP_PING };

	send_data[1] = id;
	rc = send_raw(send_data, sizeof(send_data));

	if (!rc)
	{
		u32 data_cnt;

		rc = get_rsp(NRF_DFU_OP_PING, &data_cnt);

		if (!rc)
		{
			if (data_cnt != 4 || BtDfu_RevBuf[3] != id)
			{
				printf("\r\nBad ping id!");

				rc = 1;
			}
		}
	}

	return rc;
}

//设置prn
static int req_set_prn(u16 prn)
{
	printf("%s\r\n", __func__);	

	int rc;
	u8 send_data[3] = { NRF_DFU_OP_RECEIPT_NOTIF_SET };

	printf("Set Packet Receipt Notification %u\r\n", prn);

	put_u16_le(send_data + 1, prn);
	rc = send_raw(send_data, sizeof(send_data));

	if (!rc)
	{
		u32 data_cnt;

		rc = get_rsp(NRF_DFU_OP_RECEIPT_NOTIF_SET, &data_cnt);
	}

	return rc;
}

//设置mtu
static int req_get_mtu(u16* p_mtu)
{
	printf("%s\r\n", __func__);

	int rc;
	u8 send_data[1] = { NRF_DFU_OP_MTU_GET };

	rc = send_raw(send_data, sizeof(send_data));

	if (!rc)
	{
		u32 data_cnt;

		rc = get_rsp(NRF_DFU_OP_MTU_GET, &data_cnt);		

		if (!rc)
		{
			if (data_cnt == 5)			//如果接收的数据长度 == 5，则认为数据有效
			{
				u16 mtu = get_u16_le(BtDfu_RevBuf + 3);		//提取mtu数据

				*p_mtu = mtu;

				printf("MTU: %d\r\n", mtu);
			}
			else
			{
				printf("\r\nInvalid MTU!");

				rc = 1;
			}
		}
	}

	return rc;
}


int dfu_host_setup(void)
{
	int rc;

	ping_id++;

	rc = req_ping(ping_id);

	if (!rc)
	{
		rc = req_set_prn(prn);		//设置PRN
	}

	if (!rc)
	{
		rc = req_get_mtu(&mtu);		//请求MTU（nrf52单包最大接收数据）
	}

	return rc;
}



//dfu升级协议中，定义需要首先发送ip包，再发送固件包
static int dfu_file_send(u32 ip_addr, u32 ip_size, u32 fw_addr, u32 fw_size)
{
	int err_code;

	u8* p_ip_data = NULL;
	u8* p_fw_data = NULL;

	p_ip_data = (u8*)ip_addr;
	p_fw_data = (u8*)fw_addr;

	err_code = dfu_host_setup();			//设置PRN为0，并且获取52单包最大接收长度

	if (!err_code) {
		err_code = dfu_host_send_ip(p_ip_data, ip_size);
	}

	if (!err_code) {
		err_code = dfu_host_send_fw(p_fw_data, fw_size);
	}

	return err_code;
}




//发送初始化数据包
int dfu_host_send_ip(u8* p_data, u32 data_size)
{
	int rc = 0;
	u32 crc_32 = 0;
	nrf_dfu_response_select_t rsp_select;
	nrf_dfu_response_select_t rsp_recover;

	printf("Sending init packet...\r\n");

	if (p_data == NULL || !data_size)
	{
		printf("Invalid init packet!");

		rc = 1;
	}

	if (!rc)
	{
		rc = req_obj_select(0x01, &rsp_select);		//发送06 01 C0 来发送主机IP,并且获取返回参数到rsp_select
	}

	if (!rc)
	{
		rc = try_recover_ip(p_data, data_size, &rsp_recover, &rsp_select);

		if (!rc && rsp_recover.offset == data_size)
			return rc;
	}

	if (!rc)
	{
		if (data_size > rsp_select.max_size)
		{
			printf("Init packet too big!\r\n");

			rc = 1;
		}
	}

	if (!rc)
	{
		rc = req_obj_create(0x01, data_size);
	}

	if (!rc)
	{
		rc = stream_data_crc(p_data, data_size, 0, &crc_32);
	}

	if (!rc)
	{
		rc = req_obj_execute();
	}

	return rc;
}


//查询对象
static int req_obj_select(u8 obj_type, nrf_dfu_response_select_t* p_select_rsp)
{
	int rc;
	u8 send_data[2] = { NRF_DFU_OP_OBJECT_SELECT };

	printf("Selecting Object: type:%u\r\n", obj_type);

	send_data[1] = obj_type;
	rc = send_raw(send_data, sizeof(send_data));		//obj_type = 1时发送 06 01 C0

	if (!rc)
	{
		u32 data_cnt;

		rc = get_rsp(NRF_DFU_OP_OBJECT_SELECT, &data_cnt);

		if (!rc)
		{
			if (data_cnt == 15)
			{
				p_select_rsp->max_size = get_u32_le(BtDfu_RevBuf + 3);
				p_select_rsp->offset   = get_u32_le(BtDfu_RevBuf + 7);
				p_select_rsp->crc      = get_u32_le(BtDfu_RevBuf + 11);

				printf("Object selected:  max_size:%u offset:%u crc:0x%08X\r\n", p_select_rsp->max_size, p_select_rsp->offset, p_select_rsp->crc);
			}
			else
			{
				printf("Invalid object response!\r\n");

				rc = 1;
			}
		}
	}

	return rc;
}

//创建选定对象
static int req_obj_create(u8 obj_type, u32 obj_size)
{
	printf("%s", __func__);	

	int rc;
	u8 send_data[6] = { NRF_DFU_OP_OBJECT_CREATE };

	send_data[1] = obj_type;
	put_u32_le(send_data + 2, obj_size);			//将32位的obj_size分解成4个字节到数组的4个元素
	rc = send_raw(send_data, sizeof(send_data));

	if (!rc)
	{
		u32 data_cnt;

		rc = get_rsp(NRF_DFU_OP_OBJECT_CREATE, &data_cnt);
	}

	return rc;
}


//发送数据
static int stream_data(const u8* p_data, u32 data_size)
{
	printf("%s\r\n", __func__);	

	int rc = 0;
	u32 pos, stp;
	u32 stp_max = 5;

	if (p_data == NULL || !data_size)
	{
		rc = 1;
	}

	if (!rc)
	{
		if (mtu >= 5)
		{
			stp_max = (mtu - 1) / 2 - 1;
		}
		else
		{
			printf("\r\nMTU is too small to send data!");

			rc = 1;

			return 1;
		}
	}

	for (pos = 0; !rc && pos < data_size; pos += stp)
	{
		Bt_SendBuf[0] = NRF_DFU_OP_OBJECT_WRITE;

		stp = MIN((data_size - pos), stp_max);
		memcpy(Bt_SendBuf + 1, p_data + pos, stp);

		rc = send_raw((u8 *)Bt_SendBuf, stp + 1);
	}

	return rc;
}

static int stream_data_crc(const u8* p_data, u32 data_size, u32 pos, u32* p_crc)
{
	printf("%s\r\n", __func__);

	int rc;
	nrf_dfu_response_crc_t rsp_crc;

	printf("Streaming Data: len:%u offset:%u crc:0x%08X\r\n", data_size, pos, *p_crc);

	rc = stream_data(p_data, data_size);            

	if (!rc)
	{
		*p_crc = crc32_compute(p_data, data_size, p_crc);

		rc = req_get_crc(&rsp_crc);
	}

	if (!rc)
	{
		if (rsp_crc.offset != pos + data_size)
		{
			printf("\r\nInvalid offset (%u -> %u)!", pos + data_size, rsp_crc.offset);

			rc = 2;
		}
		if (rsp_crc.crc != *p_crc)
		{
			printf("\r\nInvalid CRC (0x%08X -> 0x%08X)!", *p_crc, rsp_crc.crc);

			rc = 2;
		}
	}

	return rc;
}


static int req_get_crc(nrf_dfu_response_crc_t* p_crc_rsp)
{
	printf("%s\r\n", __func__);

	int rc;
	u8 send_data[1] = { NRF_DFU_OP_CRC_GET };

	rc = send_raw(send_data, sizeof(send_data));

	if (!rc)
	{
		u32 data_cnt;

		rc = get_rsp(NRF_DFU_OP_CRC_GET, &data_cnt);

		if (!rc)
		{
			if (data_cnt == 11)
			{
				p_crc_rsp->offset = get_u32_le(BtDfu_RevBuf + 3);
				p_crc_rsp->crc    = get_u32_le(BtDfu_RevBuf + 7);
			}
			else
			{
				printf("\r\nInvalid CRC response!");

				rc = 1;
			}
		}
	}

	return rc;
}

static int req_obj_execute(void)
{
	printf("%s\r\n", __func__);

	int rc;
	u8 send_data[1] = { NRF_DFU_OP_OBJECT_EXECUTE };

	rc = send_raw(send_data, sizeof(send_data));

	if (!rc)
	{
		u32 data_cnt;

		rc = get_rsp(NRF_DFU_OP_OBJECT_EXECUTE, &data_cnt);
	}

	return rc;
}



static int try_recover_fw(const u8* p_data, u32 data_size,
						  nrf_dfu_response_select_t* p_rsp_recover,
						  const nrf_dfu_response_select_t* p_rsp_select)
{
	printf("%s\r\n", __func__);

	int rc = 0;
	u32 max_size, stp_size;
	u32 pos_start, len_remain;
	u32 crc_32;
	int obj_exec = 1;

	*p_rsp_recover = *p_rsp_select;

	pos_start = p_rsp_recover->offset;

	if (pos_start > data_size)
	{
		printf("\r\nInvalid firmware offset reported!");

		rc = 1;
	}
	else if (pos_start > 0)
	{
		max_size = p_rsp_select->max_size;
		crc_32 = crc32_compute(p_data, pos_start, NULL);
		len_remain = pos_start % max_size;

		if (p_rsp_select->crc != crc_32)
		{
			pos_start -= ((len_remain > 0) ? len_remain : max_size);
			p_rsp_recover->offset = pos_start;

			return rc;
		}

		if (len_remain > 0)
		{
			stp_size = max_size - len_remain;

			rc = stream_data_crc(p_data + pos_start, stp_size, pos_start, &crc_32);
			if (!rc)
			{
				pos_start += stp_size;
			}
			else if (rc == 2)
			{
				rc = 0;

				pos_start -= len_remain;

				obj_exec = 0;
			}

			p_rsp_recover->offset = pos_start;
		}

		if (!rc && obj_exec)
		{
			rc = req_obj_execute();
		}
	}

	return rc;
}


int dfu_host_send_fw(const u8* p_data, u32 data_size)
{
	int rc = 0;
	u32 max_size, stp_size, pos;
	u32 crc_32 = 0;
	nrf_dfu_response_select_t rsp_select;
	nrf_dfu_response_select_t rsp_recover;
	u32 pos_start;

	printf("Sending firmware file...\r\n");

	if (p_data == NULL || !data_size)
	{
		printf("\r\nInvalid firmware data!");

		rc = 1;
	}

	if (!rc)
	{
		rc = req_obj_select(0x02, &rsp_select);		//发送 06 02 C0来选择对象
	}

	if (!rc)
	{
		rc = try_recover_fw(p_data, data_size, &rsp_recover, &rsp_select);
	}

	if (!rc)
	{
		max_size = rsp_select.max_size;

		pos_start = rsp_recover.offset;
		crc_32 = crc32_compute(p_data, pos_start, &crc_32);

		for (pos = pos_start; pos < data_size; pos += stp_size)
		{
			stp_size = MIN((data_size - pos), max_size);

			rc = req_obj_create(0x02, stp_size);

			if (!rc)
			{
				rc = stream_data_crc(p_data + pos, stp_size, pos, &crc_32);
			}

			if (!rc)
			{
				rc = req_obj_execute();
			}

			if (rc)
				break;
		}
	}

	return rc;
}

void serial_dfu_start(void)
{

	u32 ip_addr = 0;
	u32 ip_size = 0;
	u32 fw_addr = 0;
	u32 fw_size = 0;

    u8 To_Pin_App_buf[6] = {0x59,0x01,0x00,0x32,0x21,0x21};
    u8 To_Pin_Bl_buf[6]  = {0x59,0x01,0x00,0x31,0x42,0x11};

	u8 STEP_0[3]  = {0x09,0x01,0xC0};
	u8 STEP_1[3]  = {0x09,0x02,0xC0};
	u8 STEP_2[4]  = {0x02,0x00,0x00,0xC0};
	u8 STEP_3[2]  = {0x07,0xC0};
	u8 STEP_4[3]  = {0x06,0x01,0xC0};
	u8 STEP_5[7]  = {0x01,0x01,0x8D,0x00,0x00,0x00,0xC0};


	dfu_file_info(&ip_addr, &ip_size, &fw_addr, &fw_size);		//获取文件文件地址，大小等信息

    UART_Send(USART4,To_Pin_App_buf,6);
    delay_ms(200);

    UART_Send(USART4,To_Pin_Bl_buf,6);
    delay_ms(200); 

	UART_Send(USART4,STEP_0,sizeof(STEP_0));	
    delay_ms(200); 

	UART_Send(USART4,STEP_0,sizeof(STEP_0));	
    delay_ms(200); 

	UART_Send(USART4,STEP_1,sizeof(STEP_1));	
    delay_ms(200); 

		UART_Send(USART4,STEP_2,sizeof(STEP_2));	
    delay_ms(200); 

		UART_Send(USART4,STEP_3,sizeof(STEP_3));	
    delay_ms(200); 

		UART_Send(USART4,STEP_4,sizeof(STEP_4));	
    delay_ms(200); 

		UART_Send(USART4,STEP_5,sizeof(STEP_5));	
    delay_ms(200); 

	memset(Uart4Buf,0,sizeof(Uart4Buf));

	dfu_file_send(ip_addr, ip_size, fw_addr, fw_size);		//开始发送nrf52固件

}







