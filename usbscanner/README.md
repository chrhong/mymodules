```
//80 06 00 01 00 00 12 00
//12 01 [10 01] 00 00 00 40

typedef struct _USB_DEVICE_DESCRIPTOR {
    UCHAR bLength;
    UCHAR bDescriptorType;
    USHORT bcdUSB;          //USB Protocol Version
    UCHAR bDeviceClass;
    UCHAR bDeviceSubClass;
    UCHAR bDeviceProtocol;
    UCHAR bMaxPacketSize0;
    USHORT idVendor;
    USHORT idProduct;
    USHORT bcdDevice;
    UCHAR iManufacturer;
    UCHAR iProduct;
    UCHAR iSerialNumber;
    UCHAR bNumConfigurations;
} USB_DEV_DESCR, *PUSB_DEV_DESCR;

//80 06 00 02 00 00 09 00
//09 02 [22 00] 01 01 00 80

//80 06 00 02 00 00 22 00
//09 02 [22 00] 01 01 00 80
typedef struct _USB_CONFIG_DESCRIPTOR {
    UCHAR bLength;
    UCHAR bDescriptorType;
    USHORT wTotalLength;
    UCHAR bNumInterfaces;
    UCHAR bConfigurationValue;
    UCHAR iConfiguration;
    UCHAR bmAttributes;
    UCHAR MaxPower;
} USB_CFG_DESCR, *PUSB_CFG_DESCR;


/*
6. 收到设置描述符包
Setup m=0,n=0,val=45
00 09 01 00 00 00 00 00
REQUEST_STANDARD=0x9
USB_SetConfiguration WB.L =1
USB_Configure(TRUE)
USB_SetConfiguration true
由上面可以知道经过这么多次来回后，主控器已经配置完成，对这个设备可以使用了。这时，如果在WINDOWS里就会看到可以设备安装完成，可以使用了。
7. 收到设置空闲描述符包
Setup m=0,n=0,val=37
21 0A 00 00 00 00 00 00
收到这个描述符，就表明设备在空闲状态。

Set report: This is the last message
21 09 00 02 00 00 01 00 00
*/

//Get the Interface description
//81 06 00 22 00 00 7F 00
//05 01 09 06 a1 01 05 07
typedef struct _USB_INTERF_DESCRIPTOR {
    UCHAR bLength;
    UCHAR bDescriptorType;
    UCHAR bInterfaceNumber;
    UCHAR bAlternateSetting;
    UCHAR bNumEndpoints;
    UCHAR bInterfaceClass;
    UCHAR bInterfaceSubClass;
    UCHAR bInterfaceProtocol;
    UCHAR iInterface;
} USB_ITF_DESCR, *PUSB_ITF_DESCR;
```
