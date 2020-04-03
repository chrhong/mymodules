
/******************************************************************************
* Pins: PA2 == TX ; PA3 == RX ; PC9 == INT;
* 波特率默认配置为9600.
******************************************************************************/

#include "CH376_UART.H"

/* Write cmd to CH376 */
void xWriteCH376Cmd( u8 mCmd )
{
	delay_ms(2);
	USART_SendData(USART2, SER_SYNC_CODE1);
	while(USART_GetFlagStatus(USART2,USART_FLAG_TXE)==RESET);
	USART_SendData(USART2, SER_SYNC_CODE2);
	while(USART_GetFlagStatus(USART2,USART_FLAG_TXE)==RESET);
	USART_SendData(USART2, (u16)mCmd);
	while(USART_GetFlagStatus(USART2,USART_FLAG_TXE)==RESET);
	delay_us(2);
}

/* Write data to CH376 */
void xWriteCH376Data( u8 mData )
{
	USART_SendData(USART2, (u16)mData);
	while(USART_GetFlagStatus(USART2,USART_FLAG_TXE)==RESET);
	delay_us(1);
}

/* Read data from CH376 */
u8	xReadCH376Data( void )
{
	delay_us(1);
	while(USART_GetFlagStatus(USART2,USART_FLAG_RXNE)==RESET);
	return( (u8)USART_ReceiveData(USART2) );
}

void CH376_IO_Init( void )
{
	//GPIO端口设置
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO|RCC_APB2Periph_GPIOC|RCC_APB2Periph_GPIOD, ENABLE);	//使能AFIO，GPIOC,GPIOD时钟

 	USART_DeInit(USART2);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;	//INT
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	//	USART_ClockInitTypeDef USART_ClockInitStructure;

	/* config USART2 clock */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA , ENABLE);
	/* USART2 GPIO config */
	/* Configure USART2 Tx (PA.02) as alternate function push-pull */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	/* Configure USART2 Rx (PA.03) as input floating */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* Enable the USART2 Interrupt */

	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	USART_InitStructure.USART_BaudRate = 9600; //CH376 default rate
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No ;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART2, &USART_InitStructure);

#if EN_USART2_RX_INT
	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);	/* enable RX interrupt */
#endif

	USART_Cmd(USART2, ENABLE);
}

#if EN_USART2_RX_INT
void USART2_IRQHandler(void)
{

}
#endif

/********************************************************************
 * USB device related interface.
 ********************************************************************/
const u8 GetDevDescr_Array[] = {0x80, 0x06, 0x00, 0x01, 0x00, 0x00, 0x12, 0x00};
u8 buffer[64]; //the receive/send buffer
/* 主机操作 */
u8 endp_out_addr;	/* 打印机数据接收端点的端点地址 */
u8 endp_out_size;	/* 打印机数据接收端点的端点尺寸 */
BOOL1	tog_send;				/* 打印机数据接收端点的同步标志 */
u8 endp_in_addr;		/* 双向打印机发送端点的端点地址,一般不用 */
BOOL1	tog_recv;				/* 双向打印机发送端点的同步标志,一般不用 */

u8 set_usb_mode( u8 mode ) {
	u8 i;
	xWriteCH376Cmd( CMD_SET_USB_MODE );
	xWriteCH376Data( mode );
	for( i=0; i!=100; i++ ) {  /* wait cmd succeed,<30us */
		if ( xReadCH376Data()==CMD_RET_SUCCESS ) return( GLO_TRUE );
	}

	printf("set_usb_mode(%x) failed!\n", mode);
	return( GLO_FALSE );  /* CH375出错,例如芯片型号错或者处于串口方式或者不支持 */
}

u8 wait_interrupt(void) {
	u16 i;

    //wait CH375 INT return, low level means interrupt come
	for ( i = 0; CH375_INT_WIRE != 0; i ++ ) {
		delay_us(1);
		if ( i == 0xF000 )
		{
			 // >61ms, force stop. Should return USB_INT_RET_NAK
			xWriteCH376Cmd( CMD_ABORT_NAK );
			printf("Wait device timeout, send cmd abort!\n");
		}
	}

	xWriteCH376Cmd( CMD_GET_STATUS );  /* get interrupt status */
	return( xReadCH376Data() );
}

/* Data sync */
/* USB的数据同步通过切换DATA0和DATA1实现: 在设备端, USB打印机可以自动切换;
   在主机端, 必须由SET_ENDP6和SET_ENDP7命令控制CH375切换DATA0与DATA1.
   主机端的程序处理方法是为设备端的各个端点分别提供一个全局变量,
   初始值均为DATA0, 每执行一次成功事务后取反, 每执行一次失败事务后将其复位为DATA1 */

void toggle_recv( BOOL1 tog ) {  /* 主机接收同步控制:0=DATA0,1=DATA1 */
	xWriteCH376Cmd( CMD_SET_ENDP6 );
	xWriteCH376Data( tog ? 0xC0 : 0x80 );
	delay_us(2);
}

void toggle_send( BOOL1 tog ) {  /* 主机发送同步控制:0=DATA0,1=DATA1 */
	xWriteCH376Cmd( CMD_SET_ENDP7 );
	xWriteCH376Data( tog ? 0xC0 : 0x80 );
	delay_us(2);
}

u8 clr_stall( u8 endp_addr ) {  /* USB通讯失败后,复位设备端的指定端点到DATA0 */
	xWriteCH376Cmd( CMD_CLR_STALL );
	xWriteCH376Data( endp_addr );
	return( wait_interrupt() );
}

/* Data W/R, read the data buffer in CH376 */
u8 rd_usb_data( u8 *buf ) {
	u8 i, len=0;
	xWriteCH376Cmd( CMD_RD_USB_DATA );  /* 从CH375的端点缓冲区读取接收到的数据 */
	len=xReadCH376Data();  /* 后续数据长度 */
	for ( i=0; i!=len; i++ )
	{
		*buf++=xReadCH376Data();
		printf("get a data %#x\n", *buf);
	}
	return( len );
}

void wr_usb_data( u8 len, u8 *buf ) {  /* 向CH37X写入数据块 */
	xWriteCH376Cmd( CMD_WR_USB_DATA7 );  /* 向CH375的端点缓冲区写入准备发送的数据 */
	xWriteCH376Data( len );  /* 后续数据长度, len不能大于64 */
	while( len-- ) xWriteCH376Data( *buf++ );
}

u8 issue_token( u8 endp_and_pid ) {  /* 执行USB事务 */
/* 执行完成后, 将产生中断通知单片机, 如果是USB_INT_SUCCESS就说明操作成功 */
	xWriteCH376Cmd( CMD_ISSUE_TOKEN );
	xWriteCH376Data( endp_and_pid );  /* 高4位目的端点号, 低4位令牌PID */
	return( wait_interrupt() );  /* 等待CH375操作完成 */
}

u8 issue_token_X( u8 endp_and_pid, u8 tog ) {  /* 执行USB事务,适用于CH375A */
/* 执行完成后, 将产生中断通知单片机, 如果是USB_INT_SUCCESS就说明操作成功 */
	xWriteCH376Cmd( CMD_ISSUE_TKN_X );
	xWriteCH376Data( tog );  /* 同步标志的位7为主机端点IN的同步触发位, 位6为主机端点OUT的同步触发位, 位5~位0必须为0 */
	xWriteCH376Data( endp_and_pid );  /* 高4位目的端点号, 低4位令牌PID */
	return( wait_interrupt() );  /* 等待CH375操作完成 */
}

void soft_reset_print( ) {  /* 控制传输:软复位打印机 */
	tog_send=tog_recv=0;  /* 复位USB数据同步标志 */
	toggle_send( 0 );  /* SETUP阶段为DATA0 */
	buffer[0]=0x21; buffer[1]=2; buffer[2]=buffer[3]=buffer[4]=buffer[5]=buffer[6]=buffer[7]=0;  /* SETUP数据,SOFT_RESET */
	wr_usb_data( 8, buffer );  /* SETUP数据总是8字节 */
	if ( issue_token( ( 0 << 4 ) | DEF_USB_PID_SETUP )==USB_INT_SUCCESS ) {  /* SETUP阶段操作成功 */
		toggle_recv( 1 );  /* STATUS阶段,准备接收DATA1 */
		if ( issue_token( ( 0 << 4 ) | DEF_USB_PID_IN )==USB_INT_SUCCESS ) return;  /* STATUS阶段操作成功,操作成功返回 */
	}
}

void send_data( u16 len, u8 *buf ) {  /* 主机发送数据块,一次最多64KB */
	u8 l, s;
	while( len ) {  /* 连续输出数据块给USB打印机 */
		toggle_send( tog_send );  /* 数据同步 */
		l = len>endp_out_size?endp_out_size:len;  /* 单次发送不能超过端点尺寸 */
		wr_usb_data( l, buf );  /* 将数据先复制到CH375芯片中 */
		s = issue_token( ( endp_out_addr << 4 ) | DEF_USB_PID_OUT );  /* 请求CH375输出数据 */
		if ( s==USB_INT_SUCCESS ) {  /* CH375成功发出数据 */
			tog_send = ~ tog_send;  /* 切换DATA0和DATA1进行数据同步 */
			len-=l;  /* 计数 */
			buf+=l;  /* 操作成功 */
		}
		else if ( s==USB_INT_RET_NAK ) {  /* USB打印机正忙,如果未执行SET_RETRY命令则CH375自动重试,所以不会返回USB_INT_RET_NAK状态 */
			/* USB打印机正忙,正常情况下应该稍后重试 */
			/* s=get_port_status( );  如果有必要,可以检查是什么原因导致打印机忙 */
		}
		else {  /* 操作失败,正常情况下不会失败 */
			clr_stall( endp_out_addr );  /* 清除打印机的数据接收端点,或者 soft_reset_print() */
/*			soft_reset_print();  打印机出现意外错误,软复位 */
			tog_send = 0;  /* 操作失败 */
		}
/* 如果数据量较大,可以定期调用get_port_status()检查打印机状态 */
	}
}

void recv_data( u16 len, u8 *buf ) {  /* 主机接受数据块,一次最多64KB */
	u8 l, s;
	while( len ) {  /* 连续输出数据块给USB打印机 */
		toggle_recv( tog_recv );  /* 数据同步 */
		l = len>endp_out_size?endp_out_size:len;  /* 单次发送不能超过端点尺寸 */
		rd_usb_data( buf );  /* 将数据先复制到CH375芯片中 */
		s = issue_token( ( endp_out_addr << 4 ) | DEF_USB_PID_IN );  /* 请求CH375输出数据 */
		if ( s==USB_INT_SUCCESS ) {  /* CH375成功发出数据 */
			tog_recv = ~ tog_recv;  /* 切换DATA0和DATA1进行数据同步 */
			len-=l;  /* 计数 */
			buf+=l;  /* 操作成功 */
		}
		else if ( s==USB_INT_RET_NAK ) {  /* USB打印机正忙,如果未执行SET_RETRY命令则CH375自动重试,所以不会返回USB_INT_RET_NAK状态 */
			/* USB打印机正忙,正常情况下应该稍后重试 */
			/* s=get_port_status( );  如果有必要,可以检查是什么原因导致打印机忙 */
		}
		else {  /* 操作失败,正常情况下不会失败 */
			clr_stall( endp_out_addr );  /* 清除打印机的数据接收端点,或者 soft_reset_print() */
/*			soft_reset_print();  打印机出现意外错误,软复位 */
			tog_recv = 0;  /* 操作失败 */
		}

		printf("TEST>>>>>>>%s",buf);
/* 如果数据量较大,可以定期调用get_port_status()检查打印机状态 */
	}
}

u8 get_port_status( ) {  /* 查询打印机端口状态,返回状态码,如果为0FFH则说明操作失败 */
/* 返回状态码中: 位5(Paper Empty)为1说明无纸, 位4(Select)为1说明打印机联机, 位3(Not Error)为0说明打印机出错 */
	toggle_send( 0 );  /* 下面通过控制传输获取打印机的状态, SETUP阶段为DATA0 */
	buffer[0]=0xA1; buffer[1]=1; buffer[2]=buffer[3]=buffer[4]=buffer[5]=0; buffer[6]=1; buffer[7]=0;  /* SETUP数据,GET_PORT_STATUS */
	wr_usb_data( 8, buffer );  /* SETUP数据总是8字节 */
	if ( issue_token( ( 0 << 4 ) | DEF_USB_PID_SETUP )==USB_INT_SUCCESS ) {  /* SETUP阶段操作成功 */
		toggle_recv( 1 );  /* DATA阶段,准备接收DATA1 */
		if ( issue_token( ( 0 << 4 ) | DEF_USB_PID_IN )==USB_INT_SUCCESS ) {  /* DATA阶段操作成功 */
			rd_usb_data( buffer );  /* 读出接收到的数据,通常只有1个字节 */
			toggle_send( 1 );  /* STATUS阶段为DATA1 */
			wr_usb_data( 0, buffer );  /* 发送0长度的数据说明控制传输成功 */
			if ( issue_token( ( 0 << 4 ) | DEF_USB_PID_OUT )==USB_INT_SUCCESS ) return( buffer[0] );  /* 返回状态码 */
		}
	}
	return( 0xFF );  /* 返回操作失败 */
}

u8 get_port_status_X( ) {  /* 查询打印机端口状态,返回状态码,如果为0FFH则说明操作失败,适用于CH375A */
/* 返回状态码中: 位5(Paper Empty)为1说明无纸, 位4(Select)为1说明打印机联机, 位3(Not Error)为0说明打印机出错 */
	buffer[0]=0xA1; buffer[1]=1; buffer[2]=buffer[3]=buffer[4]=buffer[5]=0; buffer[6]=1; buffer[7]=0;  /* 控制传输获取打印机状态,SETUP数据 */
	wr_usb_data( 8, buffer );  /* SETUP数据总是8字节 */
	if ( issue_token_X( ( 0 << 4 ) | DEF_USB_PID_SETUP, 0x00 )==USB_INT_SUCCESS ) {  /* SETUP阶段DATA0操作成功 */
		if ( issue_token_X( ( 0 << 4 ) | DEF_USB_PID_IN, 0x80 )==USB_INT_SUCCESS ) {  /* DATA阶段DATA1接收操作成功 */
			rd_usb_data( buffer );  /* 读出接收到的数据,通常只有1个字节 */
			wr_usb_data( 0, buffer );  /* 发送0长度的数据DATA1说明控制传输成功 */
			if ( issue_token_X( ( 0 << 4 ) | DEF_USB_PID_OUT, 0x40 )==USB_INT_SUCCESS ) return( buffer[0] );  /* STATUS阶段操作成功,返回状态码 */
		}
	}
	return( 0xFF );  /* 返回操作失败 */
}

u8 get_descr( u8 type ) {  /* 从设备端获取描述符 */
	xWriteCH376Cmd( CMD_GET_DESCR );
	xWriteCH376Data( type );  /* 描述符类型, 只支持1(设备)或者2(配置) */
	printf("write -->cmd=%x, type=%x\n",CMD_GET_DESCR, type);
	return( wait_interrupt() );
}

u8 set_addr( u8 addr ) {  /* 设置设备端的USB地址 */
	u8 status;
	xWriteCH376Cmd( CMD_SET_ADDRESS );  /* 设置USB设备端的USB地址 */
	xWriteCH376Data( addr );  /* 地址, 从1到127之间的任意值, 常用2到20 */
	status=wait_interrupt();  /* 等待CH375操作完成 */
	if ( status==USB_INT_SUCCESS ) {  /* 操作成功 */
		xWriteCH376Cmd( CMD_SET_USB_ADDR );  /* 设置USB主机端的USB地址 */
		xWriteCH376Data( addr );  /* 当目标USB设备的地址成功修改后,应该同步修改主机端的USB地址 */
	}
	delay_ms(5);
	return( status );
}

u8 set_speed( u8 speed )
{
	u8 status, devSpeed;
	xWriteCH376Cmd( CMD_GET_DEV_RATE );
	devSpeed = xWriteCH376Data( 0x07 );
	status=wait_interrupt();
	if ( status==USB_INT_SUCCESS ) {
		printf("Read device speed type %x\n", devSpeed);
		xWriteCH376Cmd( CMD_SET_USB_SPEED );
		xWriteCH376Data( speed );
		status=wait_interrupt();
	}
	delay_ms(5);
	return( status );
}

u8 set_config( u8 cfg ) {  /* 设置设备端的USB配置 */
	tog_send=tog_recv=0;  /* 复位USB数据同步标志 */
	xWriteCH376Cmd( CMD_SET_CONFIG );  /* 设置USB设备端的配置值 */
	xWriteCH376Data( cfg );  /* 此值取自USB设备的配置描述符中 */
	return( wait_interrupt() );  /* 等待CH375操作完成 */
}

#define	p_dev_descr    ((PUSB_DEV_DESCR)buffer)
#define	p_cfg_descr    ((PUSB_CFG_DESCR_LONG)buffer)
u8 init_scanner() {
	u8 status, len, c;
	printf("init_scanner enter...\n");

	status=get_descr(1);  /* 获取设备描述符 */
	if ( status==USB_INT_SUCCESS ) {
		printf("get descr succeed\n");
		len=rd_usb_data( buffer );  /* 将获取的描述符数据从CH375中读出到单片机的RAM缓冲区中,返回描述符长度 */
		printf("rd_usb_data %d, %d, %d\n", len, p_dev_descr->bDescriptorType,p_dev_descr->bDeviceClass);
		while(1){}
		if ( len<18 || p_dev_descr->bDescriptorType!=1 ) return( UNKNOWN_USB_DEVICE );  /* 意外错误:描述符长度错误或者类型错误 */
		if ( p_dev_descr->bDeviceClass!=0 ) return( UNKNOWN_USB_DEVICE );  /* 连接的USB设备不是USB打印机,或者不符合USB规范 */
		status=set_addr(3);  /* 设置打印机的USB地址 */
		if ( status==USB_INT_SUCCESS ) {
			status=get_descr(2);  /* 获取配置描述符 */
			if ( status==USB_INT_SUCCESS ) {  /* 操作成功则读出描述符并分析 */
				len=rd_usb_data( buffer );  /* 将获取的描述符数据从CH375中读出到单片机的RAM缓冲区中,返回描述符长度 */
				if ( p_cfg_descr->itf_descr.bInterfaceClass!=7 || p_cfg_descr->itf_descr.bInterfaceSubClass!=1 ) return( UNKNOWN_USB_PRINT );  /* 不是USB打印机或者不符合USB规范 */
				endp_out_addr=endp_in_addr=0;
				c=p_cfg_descr->endp_descr[0].bEndpointAddress;  /* 第一个端点的地址 */
				if ( c&0x80 ) endp_in_addr=c&0x0f;  /* IN端点的地址 */
				else {  /* OUT端点 */
					endp_out_addr=c&0x0f;
					endp_out_size=p_cfg_descr->endp_descr[0].wMaxPacketSize;  /* 数据接收端点的最大包长度 */
				}
				if ( p_cfg_descr->itf_descr.bNumEndpoints>=2 ) {  /* 接口有两个以上的端点 */
					if ( p_cfg_descr->endp_descr[1].bDescriptorType==5 ) {  /* 端点描述符 */
						c=p_cfg_descr->endp_descr[1].bEndpointAddress;  /* 第二个端点的地址 */
						if ( c&0x80 ) endp_in_addr=c&0x0f;  /* IN端点 */
						else {  /* OUT端点 */
							endp_out_addr=c&0x0f;
							endp_out_size=p_cfg_descr->endp_descr[1].wMaxPacketSize;
						}
					}
				}
				if ( p_cfg_descr->itf_descr.bInterfaceProtocol<=1 ) endp_in_addr=0;  /* 单向接口不需要IN端点 */
				if ( endp_out_addr==0 ) return( UNKNOWN_USB_PRINT );  /* 不是USB打印机或者不符合USB规范 */
				status=set_config( p_cfg_descr->cfg_descr.bConfigurationValue );  /* 加载USB配置值 */
				if ( status==USB_INT_SUCCESS ) {
					xWriteCH376Cmd( CMD_SET_RETRY );  /* 设置USB事务操作的重试次数 */
					xWriteCH376Data( 0x25 );
					xWriteCH376Data( 0x89 );  /* 位7为1则收到NAK时无限重试, 位3~位0为超时后的重试次数 */
					/* 如果单片机在打印机忙时并无事可做,建议设置位7为1,使CH375在收到NAK时自动重试直到操作成功或者失败 */
					/* 如果希望单片机在打印机忙时能够做其它事,那么应该设置位7为0,使CH375在收到NAK时不重试,
						 所以在下面的USB通讯过程中,如果USB打印机正忙,issue_token等子程序将得到状态码USB_INT_RET_NAK */
				}
			}
		}
	}

	printf("get descr out, status=%#x\n",status);

	while(1)(delay_ms(100));
	return(status);
}

u8	mInitCH376Host( void )
{
	u8	res;

	CH376_IO_Init();

	//Check if CH376 coud work
	xWriteCH376Cmd( CMD_CHECK_EXIST );
	xWriteCH376Data( 0x55 );
	res = xReadCH376Data( );
	if ( res != 0xAA )
	{
	    printf("Check CH376 device failed, result=%x\n", res);
        return( GLO_FALSE );
	}

	printf("set usb mode ---> 6\n");
	set_usb_mode( 6 );
	printf("wait usb device online...\n");
	while ( wait_interrupt()!=USB_INT_CONNECT );

#define USB_RESET_FIRST 0
#if USB_RESET_FIRST
	printf("reset usb device...\n");
	set_usb_mode( 7 );
	delay_ms(10);
	set_usb_mode( 6 );
	delay_ms(100);
	printf("wait usb device online again...\n");
	while ( wait_interrupt()!=USB_INT_CONNECT );
#endif

    delay_ms(200);

	if ( init_scanner()!=USB_INT_SUCCESS ) while(1);

  return GLO_TRUE;
}