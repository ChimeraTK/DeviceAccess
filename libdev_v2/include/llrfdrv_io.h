/* 
 * File:   llrfdrv_io.h
 * Author: apiotro
 *
 * Created on 17 marzec 2012, 11:45
 */

#ifndef LLRFDRV_IO_H
#define	LLRFDRV_IO_H

#define RW_DMA       0x3
#define RW_INFO      0x4

#define RW_D8        0x0
#define RW_D16       0x1
#define RW_D32       0x2

#define LLRFDRV_IOC           			'0'
#define LLRFDRV_PHYSICAL_SLOT       _IOWR(LLRFDRV_IOC, 20, int)
#define LLRFDRV_DRIVER_VERSION      _IOWR(LLRFDRV_IOC, 21, int)
#define LLRFDRV_FIRMWARE_VERSION    _IOWR(LLRFDRV_IOC, 22, int)

#define LLRFDRV_IOC_MAXNR           22
#define LLRFDRV_IOC_MINNR           20

typedef struct _device_rw{
       u_int		offset_rw; /*offset in address*/
       u_int		data_rw;   /*data to set or returned read data */
       u_int		mode_rw;   /*mode of rw (RW_D8, RW_D16, RW_D32)*/
       u_int 		barx_rw;   /*BARx (0, 1, 2, 3)*/
       u_int 		size_rw;   /*transfer size in bytes*/
       u_int 		rsrvd_rw;  /*transfer size in bytes*/
} device_rw;

typedef struct _device_ioctrl_data  {
       u_int    offset;
       u_int    data;
       u_int    cmd;
       u_int    reserved;
}device_ioctrl_data;

#endif	/* LLRFDRV_IO_H */

