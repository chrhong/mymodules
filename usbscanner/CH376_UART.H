#include "sys.h"
#include "delay.h"
#include "usart.h" //used for printf -> USART1
#include "CH376INC.H"
#include "CH375HFM.H"

#define EN_USART2_RX_INT    0
#define CH375_INT_WIRE      PCin(9)
#define	UNKNOWN_USB_DEVICE	0xF1
#define	UNKNOWN_USB_PRINT	0xF2
#define	USB_INT_RET_NAK		0x2A		/* 00101010B,返回NAK */

#define GLO_TRUE  1
#define GLO_FALSE 0

typedef struct _USB_DEVICE_DESCRIPTOR {
    u8 bLength;
    u8 bDescriptorType;
    u16 bcdUSB;
    u8 bDeviceClass;
    u8 bDeviceSubClass;
    u8 bDeviceProtocol;
    u8 bMaxPacketSize0;
    u16 idVendor;
    u16 idProduct;
    u16 bcdDevice;
    u8 iManufacturer;
    u8 iProduct;
    u8 iSerialNumber;
    u8 bNumConfigurations;
} USB_DEV_DESCR, *PUSB_DEV_DESCR;

typedef struct _USB_CONFIG_DESCRIPTOR {
    u8 bLength;
    u8 bDescriptorType;
    u16 wTotalLength;
    u8 bNumInterfaces;
    u8 bConfigurationValue;
    u8 iConfiguration;
    u8 bmAttributes;
    u8 MaxPower;
} USB_CFG_DESCR, *PUSB_CFG_DESCR;

typedef struct _USB_INTERF_DESCRIPTOR {
    u8 bLength;
    u8 bDescriptorType;
    u8 bInterfaceNumber;
    u8 bAlternateSetting;
    u8 bNumEndpoints;
    u8 bInterfaceClass;
    u8 bInterfaceSubClass;
    u8 bInterfaceProtocol;
    u8 iInterface;
} USB_ITF_DESCR, *PUSB_ITF_DESCR;

typedef struct _USB_ENDPOINT_DESCRIPTOR {
    u8 bLength;
    u8 bDescriptorType;
    u8 bEndpointAddress;
    u8 bmAttributes;
    u8 wMaxPacketSize;
    u8 wMaxPacketSize1;
    u8 bInterval;
} USB_ENDP_DESCR, *PUSB_ENDP_DESCR;

typedef struct _USB_CONFIG_DESCRIPTOR_LONG {
	USB_CFG_DESCR	cfg_descr;
	USB_ITF_DESCR	itf_descr;
	USB_ENDP_DESCR	endp_descr[4];
} USB_CFG_DESCR_LONG, *PUSB_CFG_DESCR_LONG;


/********************************************
 * Interfaces
 ********************************************/
u8 mInitCH376Host( void );
u8 set_usb_mode( u8 mode );
u8 wait_interrupt(void);
u8 init_scanner(void);
u8 get_port_status(void);
void recv_data( u16 len, u8 *buf );