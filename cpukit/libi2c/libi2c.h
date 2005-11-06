#ifndef RTEMS_LIBI2C_H
#define RTEMS_LIBI2C_H
/*$Id$*/
#include <rtems.h>

#include <rtems/io.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Simple I2C driver API */

/* Initialize the libary - may fail if no semaphore or no driver slot is available */
int rtems_libi2c_initialize ();

/* Bus Driver API
 *
 * Bus drivers provide access to low-level i2c functions
 * such as 'send start', 'send address', 'get bytes' etc.
 */

/* first field must be a pointer to ops; driver
 * may add its own fields after this.
 * the struct that is registered with the library
 * is not copied; a pointer will we passed
 * to the callback functions (ops).
 */
typedef struct rtems_libi2c_bus_t_
{
  struct rtems_libi2c_bus_ops_ *ops;
  int size;                     /* size of whole structure */
} rtems_libi2c_bus_t;

/* Access functions a low level driver must provide;
 * 
 * All of these, except read_bytes and write_bytes
 * return RTEMS_SUCCESSFUL on success and an error status
 * otherwise. The read and write ops return the number
 * of chars read/written or -(status code) on error.
 */
typedef struct rtems_libi2c_bus_ops_
{
  /* Initialize the bus; might be called again to reset the bus driver */
  rtems_status_code (*init) (rtems_libi2c_bus_t * bushdl);
  /* Send start condition */
  rtems_status_code (*send_start) (rtems_libi2c_bus_t * bushdl);
  /* Send stop  condition */
  rtems_status_code (*send_stop) (rtems_libi2c_bus_t * bushdl);
  /* initiate transfer from (rw!=0) or to a device */
  rtems_status_code (*send_addr) (rtems_libi2c_bus_t * bushdl,
                                  uint32_t addr, int rw);
  /* read a number of bytes */
  int (*read_bytes) (rtems_libi2c_bus_t * bushdl, unsigned char *bytes,
                     int nbytes);
  /* write a number of bytes */
  int (*write_bytes) (rtems_libi2c_bus_t * bushdl, unsigned char *bytes,
                      int nbytes);
} rtems_libi2c_bus_ops_t;


/*
 * Register a lowlevel driver
 *
 * TODO: better description
 *
 * This  allocates a major number identifying *this* driver
 * (i.e., libi2c) and the minor number encodes a bus# and a i2c address.
 *
 * The name will be registered in the filesystem (parent
 * directories must exist). It may be NULL in which case
 * the library will pick a default.
 *
 * RETURNS: bus # (>=0) or -1 on error (errno set).
 */

int rtems_libi2c_register_bus (char *name, rtems_libi2c_bus_t * bus);

extern rtems_device_major_number rtems_libi2c_major;

#define RTEMS_LIBI2C_MAKE_MINOR(busno, i2caddr)	\
	((((busno)&((1<<3)-1))<<10) | ((i2caddr)&((1<<10)-1)))

/* After the library is initialized, a major number is available.
 * As soon as a low-level bus driver is registered (above routine
 * returns a 'busno'), a device node can be created in the filesystem
 * with a major/minor number pair of
 *
 *    rtems_libi2c_major / RTEMS_LIBI2C_MAKE_MINOR(busno, i2caddr)
 *
 * and a 'raw' hi-level driver is then attached to this device
 * node.
 * This 'raw' driver has very simple semantics:
 *
 *   'open'         sends a start condition
 *   'read'/'write' address the device identified by the i2c bus# and address
 *                  encoded in the minor number and read or write, respectively
 *                  a stream of bytes from or to the device. Every time the
 *                  direction is changed, a 're-start' condition followed by
 *                  an 'address' cycle is generated on the i2c bus.
 *   'close'        sends a stop condition.
 *
 * Hence, using the 'raw' driver, e.g., 100 bytes at offset 0x200 can be
 * read from an EEPROM by the following pseudo-code:
 *
 *   mknod("/dev/i2c-54", mode, MKDEV(rtems_libi2c_major, RTEMS_LIBI2C_MAKE_MINOR(0,0x54)))
 *
 *   int fd;
 *   char off[2]={0x02,0x00};
 *
 *   fd = open("/dev/i2c-54",O_RDWR);
 *   write(fd,off,2);
 *   read(fd,buf,100);
 *   close(fd);
 *
 */

/* Higher Level Driver API
 *
 * Higher level drivers know how to deal with specific i2c
 * devices (independent of the bus interface chip) and provide
 * an abstraction, i.e., the usual read/write/ioctl access.
 *
 * Using the above example, such a high level driver could
 * prevent the user from issuing potentially destructive write
 * operations (the aforementioned EEPROM interprets any 3rd
 * and following byte written to the device as data, i.e., the
 * contents could easily be changed!).
 * The correct 'read-pointer offset' programming could be 
 * implemented in 'open' and 'ioctl' of a high-level driver and
 * the user would then only have to perform harmless read
 * operations, e.g., 
 *
 *    fd = open("/dev/i2c.eeprom",O_RDONLY) / * opens and sets EEPROM read pointer * /
 *    ioctl(fd, IOCTL_SEEK, 0x200)			/ * repositions the read pointer       * /
 *    read(fd, buf, 100)
 *    close(fd)
 *
 */

/* struct provided at driver registration. The driver may store
 * private data behind the mandatory first fields but the size
 * must be set to the size of the entire struct, e.g., 
 *
 * struct driver_pvt {
 * 	rtems_libi2c_drv_t pub;
 * 	struct {  ...    } pvt;
 * } my_driver = {
 * 		{  ops: my_ops,
 * 		  size: sizeof(my_driver) 
 * 		},
 * 		{ ...};
 * };
 *
 * A pointer to this struct is passed to the callback ops.
 */

typedef struct rtems_libi2c_drv_t_
{
  rtems_driver_address_table *ops;      /* the driver ops */
  int size;                     /* size of whole structure (including appended private data) */
} rtems_libi2c_drv_t;

/*
 * The high level driver must be registered with a particular
 * bus number and i2c address.
 *
 * The registration procedure also creates a filesystem node,
 * i.e., the returned minor number is not really needed.
 * 
 * If the 'name' argument is NULL, no filesystem node is
 * created (but this can be done 'manually' using rtems_libi2c_major
 * and the return value of this routine).
 *
 * RETURNS minor number (FYI) or -1 on failure
 */
int
rtems_libi2c_register_drv (char *name, rtems_libi2c_drv_t * drvtbl,
                           unsigned bus, unsigned i2caddr);

/* Operations available to high level drivers */

/* NOTES: The bus a device is attached to is LOCKED from the first send_start
 *        until send_stop is executed!
 *
 *        Bus tenure MUST NOT span multiple system calls - otherwise, a single
 *        thread could get into the protected sections (or would deadlock if the
 *		  mutex was not nestable).
 *		  E.g., consider what happens if 'open' sends a 'start' and 'close'
 *		  sends a 'stop' (i.e., the bus mutex would be locked in 'open' and
 *        released in 'close'. A single thread could try to open two devices
 *        on the same bus and would either deadlock or nest into the bus mutex
 *        and potentially mess up the i2c messages.
 *
 *        The correct way is to *always* relinquish the i2c bus (i.e., send 'stop'
 *		  from any driver routine prior to returning control to the caller.
 *		  Consult the implementation of the generic driver routines (open, close, ...)
 *		  below or the examples in i2c-2b-eeprom.c and i2c-2b-ds1621.c
 *
 * Drivers just pass the minor number on to these routines...
 */
rtems_status_code rtems_libi2c_send_start (rtems_device_minor_number minor);

rtems_status_code rtems_libi2c_send_stop (rtems_device_minor_number minor);

rtems_status_code
rtems_libi2c_send_addr (rtems_device_minor_number minor, int rw);

/* the read/write routines return the number of bytes transferred
 * or -(status_code) on error.
 */
int
rtems_libi2c_read_bytes (rtems_device_minor_number minor,
                         unsigned char *bytes, int nbytes);

int
rtems_libi2c_write_bytes (rtems_device_minor_number minor,
                          unsigned char *bytes, int nbytes);

/* Send start, send address and read bytes */
int
rtems_libi2c_start_read_bytes (uint32_t minor, unsigned char *bytes,
                               int nbytes);

/* Send start, send address and write bytes */
int
rtems_libi2c_start_write_bytes (uint32_t minor, unsigned char *bytes,
                                int nbytes);

#ifdef __cplusplus
}
#endif

#endif
