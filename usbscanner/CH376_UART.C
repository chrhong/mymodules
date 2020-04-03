
/******************************************************************************
* Pins: PA2 == TX ; PA3 == RX ; PC9 == INT;
* ������Ĭ������Ϊ9600.
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
	//GPIO�˿�����
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO|RCC_APB2Periph_GPIOC|RCC_APB2Periph_GPIOD, ENABLE);	//ʹ��AFIO��GPIOC,GPIODʱ��

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
/* �������� */
u8 endp_out_addr;	/* ��ӡ�����ݽ��ն˵�Ķ˵��ַ */
u8 endp_out_size;	/* ��ӡ�����ݽ��ն˵�Ķ˵�ߴ� */
BOOL1	tog_send;				/* ��ӡ�����ݽ��ն˵��ͬ����־ */
u8 endp_in_addr;		/* ˫���ӡ�����Ͷ˵�Ķ˵��ַ,һ�㲻�� */
BOOL1	tog_recv;				/* ˫���ӡ�����Ͷ˵��ͬ����־,һ�㲻�� */

u8 set_usb_mode( u8 mode ) {
	u8 i;
	xWriteCH376Cmd( CMD_SET_USB_MODE );
	xWriteCH376Data( mode );
	for( i=0; i!=100; i++ ) {  /* wait cmd succeed,<30us */
		if ( xReadCH376Data()==CMD_RET_SUCCESS ) return( GLO_TRUE );
	}

	printf("set_usb_mode(%x) failed!\n", mode);
	return( GLO_FALSE );  /* CH375����,����оƬ�ͺŴ����ߴ��ڴ��ڷ�ʽ���߲�֧�� */
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
/* USB������ͬ��ͨ���л�DATA0��DATA1ʵ��: ���豸��, USB��ӡ�������Զ��л�;
   ��������, ������SET_ENDP6��SET_ENDP7�������CH375�л�DATA0��DATA1.
   �����˵ĳ�����������Ϊ�豸�˵ĸ����˵�ֱ��ṩһ��ȫ�ֱ���,
   ��ʼֵ��ΪDATA0, ÿִ��һ�γɹ������ȡ��, ÿִ��һ��ʧ��������临λΪDATA1 */

void toggle_recv( BOOL1 tog ) {  /* ��������ͬ������:0=DATA0,1=DATA1 */
	xWriteCH376Cmd( CMD_SET_ENDP6 );
	xWriteCH376Data( tog ? 0xC0 : 0x80 );
	delay_us(2);
}

void toggle_send( BOOL1 tog ) {  /* ��������ͬ������:0=DATA0,1=DATA1 */
	xWriteCH376Cmd( CMD_SET_ENDP7 );
	xWriteCH376Data( tog ? 0xC0 : 0x80 );
	delay_us(2);
}

u8 clr_stall( u8 endp_addr ) {  /* USBͨѶʧ�ܺ�,��λ�豸�˵�ָ���˵㵽DATA0 */
	xWriteCH376Cmd( CMD_CLR_STALL );
	xWriteCH376Data( endp_addr );
	return( wait_interrupt() );
}

/* Data W/R, read the data buffer in CH376 */
u8 rd_usb_data( u8 *buf ) {
	u8 i, len=0;
	xWriteCH376Cmd( CMD_RD_USB_DATA );  /* ��CH375�Ķ˵㻺������ȡ���յ������� */
	len=xReadCH376Data();  /* �������ݳ��� */
	for ( i=0; i!=len; i++ )
	{
		*buf++=xReadCH376Data();
		printf("get a data %#x\n", *buf);
	}
	return( len );
}

void wr_usb_data( u8 len, u8 *buf ) {  /* ��CH37Xд�����ݿ� */
	xWriteCH376Cmd( CMD_WR_USB_DATA7 );  /* ��CH375�Ķ˵㻺����д��׼�����͵����� */
	xWriteCH376Data( len );  /* �������ݳ���, len���ܴ���64 */
	while( len-- ) xWriteCH376Data( *buf++ );
}

u8 issue_token( u8 endp_and_pid ) {  /* ִ��USB���� */
/* ִ����ɺ�, �������ж�֪ͨ��Ƭ��, �����USB_INT_SUCCESS��˵�������ɹ� */
	xWriteCH376Cmd( CMD_ISSUE_TOKEN );
	xWriteCH376Data( endp_and_pid );  /* ��4λĿ�Ķ˵��, ��4λ����PID */
	return( wait_interrupt() );  /* �ȴ�CH375������� */
}

u8 issue_token_X( u8 endp_and_pid, u8 tog ) {  /* ִ��USB����,������CH375A */
/* ִ����ɺ�, �������ж�֪ͨ��Ƭ��, �����USB_INT_SUCCESS��˵�������ɹ� */
	xWriteCH376Cmd( CMD_ISSUE_TKN_X );
	xWriteCH376Data( tog );  /* ͬ����־��λ7Ϊ�����˵�IN��ͬ������λ, λ6Ϊ�����˵�OUT��ͬ������λ, λ5~λ0����Ϊ0 */
	xWriteCH376Data( endp_and_pid );  /* ��4λĿ�Ķ˵��, ��4λ����PID */
	return( wait_interrupt() );  /* �ȴ�CH375������� */
}

void soft_reset_print( ) {  /* ���ƴ���:����λ��ӡ�� */
	tog_send=tog_recv=0;  /* ��λUSB����ͬ����־ */
	toggle_send( 0 );  /* SETUP�׶�ΪDATA0 */
	buffer[0]=0x21; buffer[1]=2; buffer[2]=buffer[3]=buffer[4]=buffer[5]=buffer[6]=buffer[7]=0;  /* SETUP����,SOFT_RESET */
	wr_usb_data( 8, buffer );  /* SETUP��������8�ֽ� */
	if ( issue_token( ( 0 << 4 ) | DEF_USB_PID_SETUP )==USB_INT_SUCCESS ) {  /* SETUP�׶β����ɹ� */
		toggle_recv( 1 );  /* STATUS�׶�,׼������DATA1 */
		if ( issue_token( ( 0 << 4 ) | DEF_USB_PID_IN )==USB_INT_SUCCESS ) return;  /* STATUS�׶β����ɹ�,�����ɹ����� */
	}
}

void send_data( u16 len, u8 *buf ) {  /* �����������ݿ�,һ�����64KB */
	u8 l, s;
	while( len ) {  /* ����������ݿ��USB��ӡ�� */
		toggle_send( tog_send );  /* ����ͬ�� */
		l = len>endp_out_size?endp_out_size:len;  /* ���η��Ͳ��ܳ����˵�ߴ� */
		wr_usb_data( l, buf );  /* �������ȸ��Ƶ�CH375оƬ�� */
		s = issue_token( ( endp_out_addr << 4 ) | DEF_USB_PID_OUT );  /* ����CH375������� */
		if ( s==USB_INT_SUCCESS ) {  /* CH375�ɹ��������� */
			tog_send = ~ tog_send;  /* �л�DATA0��DATA1��������ͬ�� */
			len-=l;  /* ���� */
			buf+=l;  /* �����ɹ� */
		}
		else if ( s==USB_INT_RET_NAK ) {  /* USB��ӡ����æ,���δִ��SET_RETRY������CH375�Զ�����,���Բ��᷵��USB_INT_RET_NAK״̬ */
			/* USB��ӡ����æ,���������Ӧ���Ժ����� */
			/* s=get_port_status( );  ����б�Ҫ,���Լ����ʲôԭ���´�ӡ��æ */
		}
		else {  /* ����ʧ��,��������²���ʧ�� */
			clr_stall( endp_out_addr );  /* �����ӡ�������ݽ��ն˵�,���� soft_reset_print() */
/*			soft_reset_print();  ��ӡ�������������,����λ */
			tog_send = 0;  /* ����ʧ�� */
		}
/* ����������ϴ�,���Զ��ڵ���get_port_status()����ӡ��״̬ */
	}
}

void recv_data( u16 len, u8 *buf ) {  /* �����������ݿ�,һ�����64KB */
	u8 l, s;
	while( len ) {  /* ����������ݿ��USB��ӡ�� */
		toggle_recv( tog_recv );  /* ����ͬ�� */
		l = len>endp_out_size?endp_out_size:len;  /* ���η��Ͳ��ܳ����˵�ߴ� */
		rd_usb_data( buf );  /* �������ȸ��Ƶ�CH375оƬ�� */
		s = issue_token( ( endp_out_addr << 4 ) | DEF_USB_PID_IN );  /* ����CH375������� */
		if ( s==USB_INT_SUCCESS ) {  /* CH375�ɹ��������� */
			tog_recv = ~ tog_recv;  /* �л�DATA0��DATA1��������ͬ�� */
			len-=l;  /* ���� */
			buf+=l;  /* �����ɹ� */
		}
		else if ( s==USB_INT_RET_NAK ) {  /* USB��ӡ����æ,���δִ��SET_RETRY������CH375�Զ�����,���Բ��᷵��USB_INT_RET_NAK״̬ */
			/* USB��ӡ����æ,���������Ӧ���Ժ����� */
			/* s=get_port_status( );  ����б�Ҫ,���Լ����ʲôԭ���´�ӡ��æ */
		}
		else {  /* ����ʧ��,��������²���ʧ�� */
			clr_stall( endp_out_addr );  /* �����ӡ�������ݽ��ն˵�,���� soft_reset_print() */
/*			soft_reset_print();  ��ӡ�������������,����λ */
			tog_recv = 0;  /* ����ʧ�� */
		}

		printf("TEST>>>>>>>%s",buf);
/* ����������ϴ�,���Զ��ڵ���get_port_status()����ӡ��״̬ */
	}
}

u8 get_port_status( ) {  /* ��ѯ��ӡ���˿�״̬,����״̬��,���Ϊ0FFH��˵������ʧ�� */
/* ����״̬����: λ5(Paper Empty)Ϊ1˵����ֽ, λ4(Select)Ϊ1˵����ӡ������, λ3(Not Error)Ϊ0˵����ӡ������ */
	toggle_send( 0 );  /* ����ͨ�����ƴ����ȡ��ӡ����״̬, SETUP�׶�ΪDATA0 */
	buffer[0]=0xA1; buffer[1]=1; buffer[2]=buffer[3]=buffer[4]=buffer[5]=0; buffer[6]=1; buffer[7]=0;  /* SETUP����,GET_PORT_STATUS */
	wr_usb_data( 8, buffer );  /* SETUP��������8�ֽ� */
	if ( issue_token( ( 0 << 4 ) | DEF_USB_PID_SETUP )==USB_INT_SUCCESS ) {  /* SETUP�׶β����ɹ� */
		toggle_recv( 1 );  /* DATA�׶�,׼������DATA1 */
		if ( issue_token( ( 0 << 4 ) | DEF_USB_PID_IN )==USB_INT_SUCCESS ) {  /* DATA�׶β����ɹ� */
			rd_usb_data( buffer );  /* �������յ�������,ͨ��ֻ��1���ֽ� */
			toggle_send( 1 );  /* STATUS�׶�ΪDATA1 */
			wr_usb_data( 0, buffer );  /* ����0���ȵ�����˵�����ƴ���ɹ� */
			if ( issue_token( ( 0 << 4 ) | DEF_USB_PID_OUT )==USB_INT_SUCCESS ) return( buffer[0] );  /* ����״̬�� */
		}
	}
	return( 0xFF );  /* ���ز���ʧ�� */
}

u8 get_port_status_X( ) {  /* ��ѯ��ӡ���˿�״̬,����״̬��,���Ϊ0FFH��˵������ʧ��,������CH375A */
/* ����״̬����: λ5(Paper Empty)Ϊ1˵����ֽ, λ4(Select)Ϊ1˵����ӡ������, λ3(Not Error)Ϊ0˵����ӡ������ */
	buffer[0]=0xA1; buffer[1]=1; buffer[2]=buffer[3]=buffer[4]=buffer[5]=0; buffer[6]=1; buffer[7]=0;  /* ���ƴ����ȡ��ӡ��״̬,SETUP���� */
	wr_usb_data( 8, buffer );  /* SETUP��������8�ֽ� */
	if ( issue_token_X( ( 0 << 4 ) | DEF_USB_PID_SETUP, 0x00 )==USB_INT_SUCCESS ) {  /* SETUP�׶�DATA0�����ɹ� */
		if ( issue_token_X( ( 0 << 4 ) | DEF_USB_PID_IN, 0x80 )==USB_INT_SUCCESS ) {  /* DATA�׶�DATA1���ղ����ɹ� */
			rd_usb_data( buffer );  /* �������յ�������,ͨ��ֻ��1���ֽ� */
			wr_usb_data( 0, buffer );  /* ����0���ȵ�����DATA1˵�����ƴ���ɹ� */
			if ( issue_token_X( ( 0 << 4 ) | DEF_USB_PID_OUT, 0x40 )==USB_INT_SUCCESS ) return( buffer[0] );  /* STATUS�׶β����ɹ�,����״̬�� */
		}
	}
	return( 0xFF );  /* ���ز���ʧ�� */
}

u8 get_descr( u8 type ) {  /* ���豸�˻�ȡ������ */
	xWriteCH376Cmd( CMD_GET_DESCR );
	xWriteCH376Data( type );  /* ����������, ֻ֧��1(�豸)����2(����) */
	printf("write -->cmd=%x, type=%x\n",CMD_GET_DESCR, type);
	return( wait_interrupt() );
}

u8 set_addr( u8 addr ) {  /* �����豸�˵�USB��ַ */
	u8 status;
	xWriteCH376Cmd( CMD_SET_ADDRESS );  /* ����USB�豸�˵�USB��ַ */
	xWriteCH376Data( addr );  /* ��ַ, ��1��127֮�������ֵ, ����2��20 */
	status=wait_interrupt();  /* �ȴ�CH375������� */
	if ( status==USB_INT_SUCCESS ) {  /* �����ɹ� */
		xWriteCH376Cmd( CMD_SET_USB_ADDR );  /* ����USB�����˵�USB��ַ */
		xWriteCH376Data( addr );  /* ��Ŀ��USB�豸�ĵ�ַ�ɹ��޸ĺ�,Ӧ��ͬ���޸������˵�USB��ַ */
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

u8 set_config( u8 cfg ) {  /* �����豸�˵�USB���� */
	tog_send=tog_recv=0;  /* ��λUSB����ͬ����־ */
	xWriteCH376Cmd( CMD_SET_CONFIG );  /* ����USB�豸�˵�����ֵ */
	xWriteCH376Data( cfg );  /* ��ֵȡ��USB�豸�������������� */
	return( wait_interrupt() );  /* �ȴ�CH375������� */
}

#define	p_dev_descr    ((PUSB_DEV_DESCR)buffer)
#define	p_cfg_descr    ((PUSB_CFG_DESCR_LONG)buffer)
u8 init_scanner() {
	u8 status, len, c;
	printf("init_scanner enter...\n");

	status=get_descr(1);  /* ��ȡ�豸������ */
	if ( status==USB_INT_SUCCESS ) {
		printf("get descr succeed\n");
		len=rd_usb_data( buffer );  /* ����ȡ�����������ݴ�CH375�ж�������Ƭ����RAM��������,�������������� */
		printf("rd_usb_data %d, %d, %d\n", len, p_dev_descr->bDescriptorType,p_dev_descr->bDeviceClass);
		while(1){}
		if ( len<18 || p_dev_descr->bDescriptorType!=1 ) return( UNKNOWN_USB_DEVICE );  /* �������:���������ȴ���������ʹ��� */
		if ( p_dev_descr->bDeviceClass!=0 ) return( UNKNOWN_USB_DEVICE );  /* ���ӵ�USB�豸����USB��ӡ��,���߲�����USB�淶 */
		status=set_addr(3);  /* ���ô�ӡ����USB��ַ */
		if ( status==USB_INT_SUCCESS ) {
			status=get_descr(2);  /* ��ȡ���������� */
			if ( status==USB_INT_SUCCESS ) {  /* �����ɹ������������������ */
				len=rd_usb_data( buffer );  /* ����ȡ�����������ݴ�CH375�ж�������Ƭ����RAM��������,�������������� */
				if ( p_cfg_descr->itf_descr.bInterfaceClass!=7 || p_cfg_descr->itf_descr.bInterfaceSubClass!=1 ) return( UNKNOWN_USB_PRINT );  /* ����USB��ӡ�����߲�����USB�淶 */
				endp_out_addr=endp_in_addr=0;
				c=p_cfg_descr->endp_descr[0].bEndpointAddress;  /* ��һ���˵�ĵ�ַ */
				if ( c&0x80 ) endp_in_addr=c&0x0f;  /* IN�˵�ĵ�ַ */
				else {  /* OUT�˵� */
					endp_out_addr=c&0x0f;
					endp_out_size=p_cfg_descr->endp_descr[0].wMaxPacketSize;  /* ���ݽ��ն˵���������� */
				}
				if ( p_cfg_descr->itf_descr.bNumEndpoints>=2 ) {  /* �ӿ����������ϵĶ˵� */
					if ( p_cfg_descr->endp_descr[1].bDescriptorType==5 ) {  /* �˵������� */
						c=p_cfg_descr->endp_descr[1].bEndpointAddress;  /* �ڶ����˵�ĵ�ַ */
						if ( c&0x80 ) endp_in_addr=c&0x0f;  /* IN�˵� */
						else {  /* OUT�˵� */
							endp_out_addr=c&0x0f;
							endp_out_size=p_cfg_descr->endp_descr[1].wMaxPacketSize;
						}
					}
				}
				if ( p_cfg_descr->itf_descr.bInterfaceProtocol<=1 ) endp_in_addr=0;  /* ����ӿڲ���ҪIN�˵� */
				if ( endp_out_addr==0 ) return( UNKNOWN_USB_PRINT );  /* ����USB��ӡ�����߲�����USB�淶 */
				status=set_config( p_cfg_descr->cfg_descr.bConfigurationValue );  /* ����USB����ֵ */
				if ( status==USB_INT_SUCCESS ) {
					xWriteCH376Cmd( CMD_SET_RETRY );  /* ����USB������������Դ��� */
					xWriteCH376Data( 0x25 );
					xWriteCH376Data( 0x89 );  /* λ7Ϊ1���յ�NAKʱ��������, λ3~λ0Ϊ��ʱ������Դ��� */
					/* �����Ƭ���ڴ�ӡ��æʱ�����¿���,��������λ7Ϊ1,ʹCH375���յ�NAKʱ�Զ�����ֱ�������ɹ�����ʧ�� */
					/* ���ϣ����Ƭ���ڴ�ӡ��æʱ�ܹ���������,��ôӦ������λ7Ϊ0,ʹCH375���յ�NAKʱ������,
						 �����������USBͨѶ������,���USB��ӡ����æ,issue_token���ӳ��򽫵õ�״̬��USB_INT_RET_NAK */
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