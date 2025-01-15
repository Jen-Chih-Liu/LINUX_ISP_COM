#include <stdio.h> /* Standard input/output definitions */
#include <stdlib.h>
#include <string.h>	 /* String function definitions */
#include <unistd.h>	 /* UNIX standard function definitions */
#include <fcntl.h>	 /* File control definitions */
#include <errno.h>	 /* Error number definitions */
#include <termios.h>      
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
//this code is show mcu information
// ---- get os info ---- //
void getOsInfo(void)
{
	FILE *fp = fopen("/proc/version", "r");
	if (NULL == fp)
		printf("failed to open version\n");
	char szTest[1000] = { 0 };
	while (!feof(fp))
	{
		memset(szTest, 0, sizeof(szTest));
		fgets(szTest, sizeof(szTest) - 1, fp);    // leave out \n
		printf("%s\n\r", szTest);
	}
	fclose(fp);
}


// ---- get cpu info ---- //
void getCpuInfo(void)
{
	FILE *fp = fopen("/proc/cpuinfo", "r");
	if (NULL == fp)
		printf("failed to open cpuinfo\n");
	char szTest[1000] = { 0 };
	// read file line by line
	while(!feof(fp))
	{
		memset(szTest, 0, sizeof(szTest));
		fgets(szTest, sizeof(szTest) - 1, fp);    // leave out \n
		printf("%s\n\r", szTest);
	}
	fclose(fp);
}
// ---- get memory info ---- //
void getMemoryInfo(void)
{
	FILE *fp = fopen("/proc/meminfo", "r");
	if (NULL == fp)
		printf("failed to open meminfo\n");
	char szTest[1000] = { 0 };
	while (!feof(fp))
	{
		memset(szTest, 0, sizeof(szTest));
		fgets(szTest, sizeof(szTest) - 1, fp);
		printf("%s\n\r", szTest);
	}
	fclose(fp);
}

// #define dbg_printf printf
#define dbg_printf(...)
unsigned char send_buf[64];
unsigned char recv_buf[64];
#define Package_Size 64
unsigned int PacketNumber;
#define Time_Out_Value 1000
int com_handler;
/**
 * Opens the specified serial port.
 *
 * @param comport The name of the serial port device (e.g., "/dev/ttyUSB0")
 * @return The file descriptor for the opened port, or -1 if an error occurs
 */
int open_port(char *comport)
{
	int fd, fset;
	int save_flag;
	// fd = open("/dev/ttyUSB0", O_RDWR);
	//  Open the serial port for reading and writing
	fd = open(comport, O_RDWR);
	// Set the file status flags for the opened file descriptor
	fset = fcntl(fd, F_SETFL, 0);
	if (fset < 0)
		printf("fcntl failed!\n");
	else
		printf("fcntl = %d\n", fset);
	// Check if standard input is a terminal device
	if(isatty(STDIN_FILENO) == 0)
		printf("standard input is not a terminal device\n");
	else
		printf("isatty success!\n");

	printf("fd = %d\n", fd);

	return fd;
}

/**
 * Writes the specified data to the given file descriptor (fd).
 *
 * @param fd The file descriptor to write to
 * @param buf A pointer to the data buffer to be written
 * @param len The length of the data to be written
 * @return The number of successfully written bytes, or -1 if an error occurs
 */
int write_port(int fd, unsigned char *buf, size_t len)
{
	int num;

	num = write(fd, buf, len);
	if (num < 0)
		printf("write failed! (%s)\n", strerror(errno));
	else

		return num;
}
/**
 * Reads data from the specified serial port.
 *
 * @param fd The file descriptor of the serial port
 * @param buf A pointer to the buffer to store the read data
 * @param len The maximum length of the data to read
 * @param tout A pointer to a timeval struct specifying the timeout duration
 * @return The number of bytes read, or -1 if an error occurs
 */
int read_port(int fd, unsigned char *buf, size_t len, struct timeval *tout)
{
	fd_set inputs;
	int num, num1, ret, i, j;
	unsigned char tempbuffer[64];
	num = 0;
	j = 0;
	int total_read = 0;
	int bytes_to_read = 64;
	FD_ZERO(&inputs);
	FD_SET(fd, &inputs);

	ret = select(fd + 1, &inputs, (fd_set *)NULL, (fd_set *)NULL, tout);
	// printf("select = %d\n", ret);
	if(ret < 0)
	{
		perror("select error!!");
		return ret;
	}
	if (ret > 0)
	{
		if (FD_ISSET(fd, &inputs))
#if 0
			num = read(fd, buf, len);
		//printf("num t1:%d\n\r", num);
		//num1 = read(fd, tempbuffer, 64 - num);
		//printf("num t2:%d\n\r", num1);
#endif 
			while(total_read < bytes_to_read) {
			int bytes_read = read(fd, buf + total_read, bytes_to_read - total_read);

			if (bytes_read < 0) {
				if (errno == EAGAIN || errno == EWOULDBLOCK) {
						
					continue;
				}					
			}
			else if (total_read == 64) {					
				printf("done\n\r");
				break;
			}

			total_read += bytes_read;
		}	
	}


	return total_read;
}

typedef enum
{
	RES_FALSE          = 0,
	RES_PASS,
	RES_FILE_NO_FOUND,
	RES_PROGRAM_FALSE,
	RES_CONNECT,
	RES_CONNECT_FALSE,
	RES_CHIP_FOUND,
	RES_DISCONNECT,
	RES_FILE_LOAD,
	RES_FILE_SIZE_OVER,
	RES_TIME_OUT,
	RES_SN_OK,
	RES_DETECT_MCU,
	RES_NO_DETECT,
} ISP_STATE;

/**
 * Performs SN package UART communication.
 *
 * @return The ISP_STATE result of the communication
 */
ISP_STATE SN_PACKAGE_UART(void)
{
	struct timeval tout;
	clock_t start_time, end_time;
	float total_time = 0;
	int send_num, recv_num;
	tout.tv_sec = 1;
	tout.tv_usec = 100 * 1000;
	unsigned char cmd[Package_Size] = {
		0xa4,
		0,
		0,
		0,
		(PacketNumber & 0xff),
		((PacketNumber >> 8) & 0xff),
		((PacketNumber >> 16) & 0xff),
		((PacketNumber >> 24) & 0xff),
		(PacketNumber & 0xff),
		((PacketNumber >> 8) & 0xff),
		((PacketNumber >> 16) & 0xff),
		((PacketNumber >> 24) & 0xff) 
	};
	send_num = write_port(com_handler, cmd, Package_Size);
	if (send_num != 64)
	{
		printf("Writ flase %d bytes\n\r", send_num);
	}
	start_time = clock(); /* mircosecond */
	while (1)
	{
		recv_num = read_port(com_handler, recv_buf, Package_Size, &tout);
		if (recv_num < 0)
			printf("read failed! (%s)\n", strerror(errno));
		if (recv_num != 64)
		{
			dbg_printf("read flase %d bytes\n\r", recv_num);
		}
		dbg_printf("package: 0x%x\n\r", recv_buf[4]);
		if ((recv_buf[4] | ((recv_buf[5] << 8) & 0xff00) | ((recv_buf[6] << 16) & 0xff0000) | ((recv_buf[7] << 24) & 0xff000000)) == (PacketNumber + 1))
			break;
		end_time = clock();
		/* CLOCKS_PER_SEC is defined at time.h */
		if ((end_time - start_time) > Time_Out_Value)
			return RES_TIME_OUT;
	}
	PacketNumber += 2;
	return RES_SN_OK;
}

ISP_STATE CHECK_UART_LINK(void)
{
	struct timeval tout;
	clock_t start_time, end_time;
	float total_time = 0;
	int send_num, recv_num;
	tout.tv_sec = 1;
	tout.tv_usec = 1000;
	int i;
	unsigned char cmd[Package_Size] = {
		0xae,
		0,
		0,
		0,
		(PacketNumber & 0xff),
		((PacketNumber >> 8) & 0xff),
		((PacketNumber >> 16) & 0xff),
		((PacketNumber >> 24) & 0xff),
		(PacketNumber & 0xff),
		((PacketNumber >> 8) & 0xff),
		((PacketNumber >> 16) & 0xff),
		((PacketNumber >> 24) & 0xff) 
	};

	// start_time = clock(); /* mircosecond */
	while(1)
	{
		usleep(50000);
		send_num = write_port(com_handler, cmd, Package_Size);
		if (send_num != 64)
		{
			printf("Writ flase %d bytes\n\r", send_num);
		}
		recv_num = read_port(com_handler, recv_buf, Package_Size, &tout);
		if (recv_num < 0)
			printf("read failed! (%s)\n", strerror(errno));
		if (recv_num != 64)
		{
			dbg_printf("read flase %d bytes\n\r", recv_num);
		}
		//printf("package: 0x%x\n\r", i++);
		//printf("package: 0x%x\n\r",(recv_buf[4] | ((recv_buf[5] << 8) & 0xff00) | ((recv_buf[6] << 16) & 0xff0000) |  ((recv_buf[7] << 24) & 0xff000000));
		if ((recv_buf[4] | ((recv_buf[5] << 8) & 0xff00) | ((recv_buf[6] << 16) & 0xff0000) | ((recv_buf[7] << 24) & 0xff000000)) == (PacketNumber + 1))
			break;
		// end_time = clock();
		/* CLOCKS_PER_SEC is defined at time.h */
		// if ((end_time - start_time) > Time_Out_Value)
		// return RES_TIME_OUT;
	}
	PacketNumber += 2;
	return RES_SN_OK;
}

/**
 * Reads the firmware version via UART.
 *
 * @return The firmware version as an unsigned integer
 */
unsigned int READFW_VERSION_UART(void)
{
	struct timeval tout;
	clock_t start_time, end_time;
	float total_time = 0;
	int send_num, recv_num;
	tout.tv_sec = 1;
	tout.tv_usec = 10000 * 1000;
	unsigned char cmd[Package_Size] = {
		0xa6,
		0,
		0,
		0,
		(PacketNumber & 0xff),
		((PacketNumber >> 8) & 0xff),
		((PacketNumber >> 16) & 0xff),
		((PacketNumber >> 24) & 0xff)
	 };
	send_num = write_port(com_handler, cmd, Package_Size);
	if (send_num != 64)
	{
		printf("Writ fale %d bytes\n\r", send_num);
	}
	start_time = clock(); /* mircosecond */
	while (1)
	{
		recv_num = read_port(com_handler, recv_buf, Package_Size, &tout);
		if (recv_num != 0)
			printf("count=%d\n\r", recv_num);
		if (recv_num == 64)
		{		
			dbg_printf("package: 0x%x\n\r", recv_buf[4]);
			if ((recv_buf[4] | ((recv_buf[5] << 8) & 0xff00) | ((recv_buf[6] << 16) & 0xff0000) | ((recv_buf[7] << 24) & 0xff000000)) == (PacketNumber + 1))
				break;
		}
		//end_time = clock(); 
		/* CLOCKS_PER_SEC is defined at time.h */ 
		//if ((end_time - start_time) > Time_Out_Value)
		//	return 0;
	}
	dbg_printf("fw version:0x%x\n\r", recv_buf[8]);
	dbg_printf("\n\r");
	PacketNumber += 2;
	return (recv_buf[8] | ((recv_buf[9] << 8) & 0xff00) | ((recv_buf[10] << 16) & 0xff0000) | ((recv_buf[11] << 24) & 0xff000000));
}

/**
 * Sets the serial port options.
 *
 * @param fd The file descriptor of the serial port
 */
void set_opt(int fd)
{
	struct termios options;

	tcgetattr(fd, &options);

	cfsetispeed(&options, B115200); /* (void) is to stop warning in cygwin */

	cfsetospeed(&options, B115200);

	options.c_cflag &= ~CSIZE;

	options.c_cflag |= CS8; /* 8 bits */

	options.c_cflag &= ~CSTOPB; /* 1 stop bit */

	options.c_cflag &= ~PARENB; /* no parity */

	options.c_cflag &= ~PARODD;

	options.c_cflag &= ~CRTSCTS; /* HW flow control off */

	options.c_lflag = 0; /* RAW input */

	options.c_iflag = 0; /* SW flow control off, no parity checks etc */

	options.c_oflag &= ~OPOST; /* RAW output */

	options.c_cc[VTIME] = 0; /* 1 sec */

	options.c_cc[VMIN] = 0;

	options.c_cflag |= (CLOCAL | CREAD);

	tcsetattr(fd, TCSAFLUSH, &options);
}
/**
 * Reads the PID (Product ID) from the UART.
 *
 * @return The result of the operation (RES_PASS if successful)
 */
ISP_STATE READ_PID_UART(void)
{
	clock_t start_time, end_time;
	struct timeval tout;
	int send_num, recv_num;
	tout.tv_sec = 1;
	tout.tv_usec = 100 * 1000;
	float total_time = 0;
	unsigned char cmd[Package_Size] = {
		0xB1,
		0,
		0,
		0,
		(PacketNumber & 0xff),
		((PacketNumber >> 8) & 0xff),
		((PacketNumber >> 16) & 0xff),
		((PacketNumber >> 24) & 0xff) 
	};
	send_num = write_port(com_handler, cmd, Package_Size);
	if (send_num != 64)
	{
		printf("Writ fale %d bytes\n\r", send_num);
	}
	start_time = clock(); /* mircosecond */
	while (1)
	{
		recv_num = read_port(com_handler, recv_buf, Package_Size, &tout);
		if (recv_num < 0)
			printf("read failed! (%s)\n", strerror(errno));
		dbg_printf("package: 0x%x\n\r", recv_buf[4]);
		if ((recv_buf[4] | ((recv_buf[5] << 8) & 0xff00) | ((recv_buf[6] << 16) & 0xff0000) | ((recv_buf[7] << 24) & 0xff000000)) == (PacketNumber + 1))
			break;

		end_time = clock();
		/* CLOCKS_PER_SEC is defined at time.h */
		if ((end_time - start_time) > Time_Out_Value)
			return RES_TIME_OUT;
	}
	printf("pid: 0x%x\n\r", (recv_buf[8] | ((recv_buf[9] << 8) & 0xff00) | ((recv_buf[10] << 16) & 0xff0000) | ((recv_buf[11] << 24) & 0xff000000)));
	printf("\n\r");

	PacketNumber += 2;
#if 0
	//return (buffer[8]|((buffer[9]<<8)&0xff00)|((buffer[10]<<16)&0xff0000)|((buffer[11]<<24)&0xff000000));
	unsigned int temp_PDID = recv_buf[8] | ((recv_buf[9] << 8) & 0xff00) | ((recv_buf[10] << 16) & 0xff0000) | ((recv_buf[11] << 24) & 0xff000000);
	CPartNumItem temp;

	int i = 0, j = (sizeof(g_PartNumItems) / sizeof(g_PartNumItems[0]));
	while (1) {
		temp = g_PartNumItems[i];
		if (temp_PDID == temp.PID)
		{
			printf("Part number: %s\n\r", g_PartNumItems[i].PartNumber);
			printf("APROM SIZE: %dKB\n\r", g_PartNumItems[i].APROM);
			printf("LDROM SIZE: %dKB\n\r", g_PartNumItems[i].LDROM);
			printf("DATAFLASH SIZE: %dKB\n\r", g_PartNumItems[i].DATAFLASH);
			APROM_SIZE = g_PartNumItems[i].APROM;
			LDROM_SIZE = g_PartNumItems[i].LDROM;
			DATAFLASH_SIZE = g_PartNumItems[i].DATAFLASH;		
			break;
		}
		i++;
		if (i == j)
			return RES_FALSE;		
	}
#endif
	return RES_PASS;
}

/**
 * Reads the configuration from the UART.
 */
void READ_CONFIG_UART(void)
{
	clock_t start_time, end_time;
	struct timeval tout;
	int send_num, recv_num;
	tout.tv_sec = 1;
	tout.tv_usec = 100 * 1000;
	float total_time = 0;

	unsigned char cmd[Package_Size] = {
		0xa2,
		0,
		0,
		0,
		(PacketNumber & 0xff),
		((PacketNumber >> 8) & 0xff),
		((PacketNumber >> 16) & 0xff),
		((PacketNumber >> 24) & 0xff)
	 };
	send_num = write_port(com_handler, cmd, Package_Size);
	if (send_num != 64)
	{
		printf("Writ fale %d bytes\n\r", send_num);
	}
	start_time = clock(); /* mircosecond */
	while (1)
	{
		recv_num = read_port(com_handler, recv_buf, Package_Size, &tout);
		if (recv_num < 0)
			printf("read failed! (%s)\n", strerror(errno));
		dbg_printf("package: 0x%x\n\r", recv_buf[4]);
		if ((recv_buf[4] | ((recv_buf[5] << 8) & 0xff00) | ((recv_buf[6] << 16) & 0xff0000) | ((recv_buf[7] << 24) & 0xff000000)) == (PacketNumber + 1))
			break;
		end_time = clock();
		/* CLOCKS_PER_SEC is defined at time.h */
		if ((end_time - start_time) > Time_Out_Value)
		{
			printf("Time out\n\r");
			break;
		}
	}
	printf("config0: 0x%x\n\r", (recv_buf[8] | ((recv_buf[9] << 8) & 0xff00) | ((recv_buf[10] << 16) & 0xff0000) | ((recv_buf[11] << 24) & 0xff000000)));
	printf("config1: 0x%x\n\r", (recv_buf[12] | ((recv_buf[13] << 8) & 0xff00) | ((recv_buf[14] << 16) & 0xff0000) | ((recv_buf[15] << 24) & 0xff000000)));
	printf("\n\r");
	PacketNumber += 2;
}
/**
 * Runs the program from APROM to LDROM using UART.
 */
void RUN_TO_LDROM_UART(void)
{

	struct timeval tout;
	int send_num;

	tout.tv_sec = 1;
	tout.tv_usec = 100 * 1000;
	unsigned char cmd[Package_Size] = {
		0xAC,
		0,
		0,
		0,
		(PacketNumber & 0xff),
		((PacketNumber >> 8) & 0xff),
		((PacketNumber >> 16) & 0xff),
		((PacketNumber >> 24) & 0xff) 
	};
	send_num = write_port(com_handler, cmd, Package_Size);
	if (send_num != 64)
	{
		printf("Writ fale %d bytes\n\r", send_num);
	}
	PacketNumber += 2;
}

/**
 * Runs the program from LDROM to APROM using UART.
 */
void RUN_TO_APROM_UART(void)
{

	struct timeval tout;
	int send_num;

	tout.tv_sec = 1;
	tout.tv_usec = 100 * 1000;
	unsigned char cmd[Package_Size] = {
		0xab,
		0,
		0,
		0,
		(PacketNumber & 0xff),
		((PacketNumber >> 8) & 0xff),
		((PacketNumber >> 16) & 0xff),
		((PacketNumber >> 24) & 0xff)
	 };
	send_num = write_port(com_handler, cmd, Package_Size);
	if (send_num != 64)
	{
		printf("Writ fale %d bytes\n\r", send_num);
	}
	PacketNumber += 2;
}
int mode;
/**
 * Reads the flash run areamode using UART.
 */
void READ_FLASH_RUN_MODE(void)
{
	clock_t start_time, end_time;
	struct timeval tout;
	int send_num, recv_num;
	tout.tv_sec = 1;
	tout.tv_usec = 100 * 1000;
	float total_time = 0;

	unsigned char cmd[Package_Size] = {
		0xCA,
		0,
		0,
		0,
		(PacketNumber & 0xff),
		((PacketNumber >> 8) & 0xff),
		((PacketNumber >> 16) & 0xff),
		((PacketNumber >> 24) & 0xff) 
	};
	send_num = write_port(com_handler, cmd, Package_Size);
	if (send_num != 64)
	{
		printf("Writ fale %d bytes\n\r", send_num);
	}
	start_time = clock(); /* mircosecond */
	while (1)
	{
		recv_num = read_port(com_handler, recv_buf, Package_Size, &tout);
		if (recv_num < 0)
			printf("read failed! (%s)\n", strerror(errno));
		dbg_printf("package: 0x%x\n\r", recv_buf[4]);
		if ((recv_buf[4] | ((recv_buf[5] << 8) & 0xff00) | ((recv_buf[6] << 16) & 0xff0000) | ((recv_buf[7] << 24) & 0xff000000)) == (PacketNumber + 1))
			break;
		end_time = clock();
		/* CLOCKS_PER_SEC is defined at time.h */
		if ((end_time - start_time) > Time_Out_Value)
		{
			printf("Time out\n\r");
			break;
		}
	}
	printf("mode: 0x%x\n\r", (recv_buf[8] | ((recv_buf[9] << 8) & 0xff00) | ((recv_buf[10] << 16) & 0xff0000) | ((recv_buf[11] << 24) & 0xff000000)));
	if ((recv_buf[8] | ((recv_buf[9] << 8) & 0xff00) | ((recv_buf[10] << 16) & 0xff0000) | ((recv_buf[11] << 24) & 0xff000000)) == 2)
	{
		printf("The MCU run in LDROM\n\r");
		mode = 2;
	}
	else
	{
		printf("run in APROM");
		mode = 1;
	}
	printf("\n\r");
	PacketNumber += 2;
}

unsigned int file_totallen;
unsigned int file_checksum;
#define CMD_UPDATE_APROM 0x000000A0
/**
 * Calculates the checksum of a buffer.
 *
 * @param buf The buffer containing the data.
 * @param len The length of the buffer.
 * @return The checksum value.
 */
unsigned short Checksum(unsigned char *buf, int len)
{
	int i;
	unsigned short c;

	for (c = 0, i = 0; i < len; i++)
	{
		c += buf[i];
	}
	return (c);
}
/**
 * Copies a specified number of bytes from source to destination.
 *
 * @param dest The destination memory location.
 * @param src The source memory location.
 * @param size The number of bytes to copy.
 */
void WordsCpy(void *dest, void *src, unsigned int size)
{
	unsigned char *pu8Src, *pu8Dest;
	unsigned int i;

	pu8Dest = (unsigned char *)dest;
	pu8Src = (unsigned char *)src;

	for (i = 0; i < size; i++)
		pu8Dest[i] = pu8Src[i];
}
unsigned short gcksum;
/**
*Sends data through a port.
*
* @ return 1 if the data is sent successfully,
*          0 otherwise.
*/
int SendData(void)
{
	int Result;
	int send_num;
	gcksum = Checksum(send_buf, Package_Size);

	send_num = write_port(com_handler, send_buf, Package_Size);
	if (send_num != 64)
	{
		printf("Writ fale %d bytes\n\r", send_num);
		return 0;
	}
	return 1;
}
/**
 * Receives data from a port.
 *
 * @return 1 if the data is received successfully, 0 otherwise.
 */
int RcvData(void)
{
	clock_t start_time, end_time;
	struct timeval tout;
	int send_num, recv_num;
	tout.tv_sec = 1;
	tout.tv_usec = 100 * 1000;
	float total_time = 0;
	int Result;
	unsigned short lcksum, i;
	unsigned char *pBuf;
	start_time = clock(); /* mircosecond */
	while (1)
	{
		recv_num = read_port(com_handler, recv_buf, Package_Size, &tout);
		if (recv_num < 0)
			printf("read failed! (%s)\n", strerror(errno));
		dbg_printf("package: 0x%x\n\r", recv_buf[4]);
		if ((recv_buf[4] | ((recv_buf[5] << 8) & 0xff00) | ((recv_buf[6] << 16) & 0xff0000) | ((recv_buf[7] << 24) & 0xff000000)) == (PacketNumber))
			break;
		end_time = clock();
		/* CLOCKS_PER_SEC is defined at time.h */
		if ((end_time - start_time) > Time_Out_Value)
		{
			printf("Time out\n\r");
			break;
		}
	}

	pBuf = recv_buf;
	WordsCpy(&lcksum, pBuf, 2);
	pBuf += 4;

	// if (inpw(pBuf) != PacketNumber)
	//{
	//	dbg_printf("g_packno=%d rcv %d\n", g_packno, inpw(pBuf));
	//	Result = 0;
	// }
	// else
	//{
	if(lcksum != gcksum)
	{
		dbg_printf("gcksum=%x lcksum=%x\n", gcksum, lcksum);
		Result = 0;
		return Result;
	}
	PacketNumber++;
	Result = 1;
	//}
	return Result;
}

/**
 * Receives data from a port without a timeout mechanism.
 *
 * @return 1 if the data is received successfully, 0 otherwise.
 */

int RcvData_without_timeout(void)
{
	clock_t start_time, end_time;
	struct timeval tout;
	int send_num, recv_num;
	tout.tv_sec = 1;
	tout.tv_usec = 100 * 1000;
	float total_time = 0;
	int Result;
	unsigned short lcksum, i;
	unsigned char *pBuf;
	while (1)
	{
		recv_num = read_port(com_handler, recv_buf, Package_Size, &tout);
		if (recv_num < 0)
			printf("read failed! (%s)\n", strerror(errno));
		dbg_printf("package: 0x%x\n\r", recv_buf[4]);
		if ((recv_buf[4] | ((recv_buf[5] << 8) & 0xff00) | ((recv_buf[6] << 16) & 0xff0000) | ((recv_buf[7] << 24) & 0xff000000)) == (PacketNumber))
			break;
		// end_time = clock();
		/* CLOCKS_PER_SEC is defined at time.h */
		// if ((end_time - start_time) > Time_Out_Value)
		//{
		//	printf("Time out\n\r");
		//	break;
		//	}
	}

	pBuf = recv_buf;
	WordsCpy(&lcksum, pBuf, 2);
	pBuf += 4;

	// if (inpw(pBuf) != PacketNumber)
	//{
	//	dbg_printf("g_packno=%d rcv %d\n", g_packno, inpw(pBuf));
	//	Result = 0;
	// }
	// else
	//{
	if(lcksum != gcksum)
	{
		dbg_printf("gcksum=%x lcksum=%x\n", gcksum, lcksum);
		Result = 0;
		return Result;
	}
	PacketNumber++;
	Result = 1;
	//}
	return Result;
}

/**
 * Writes the checksum to the device and receives the response.
 */
void WRITE_CHECKSUM(void)
{
	clock_t start_time, end_time;
	struct timeval tout;
	int send_num, recv_num;
	tout.tv_sec = 1;
	tout.tv_usec = 100 * 1000;
	float total_time = 0;

	unsigned char cmd[Package_Size] = {
		0xC9,
		0,
		0,
		0,
		(PacketNumber & 0xff),
		((PacketNumber >> 8) & 0xff),
		((PacketNumber >> 16) & 0xff),
		((PacketNumber >> 24) & 0xff) 
	};
	unsigned int buffer[2];
	buffer[0] = file_totallen;
	buffer[1] = file_checksum;
	WordsCpy(cmd + 8, buffer, 4);
	WordsCpy(cmd + 12, buffer + 1, 4);
	send_num = write_port(com_handler, cmd, Package_Size);
	if (send_num != 64)
	{
		printf("Writ fale %d bytes\n\r", send_num);
	}
	start_time = clock(); /* mircosecond */
	while (1)
	{
		recv_num = read_port(com_handler, recv_buf, Package_Size, &tout);
		if (recv_num < 0)
			printf("read failed! (%s)\n", strerror(errno));
		dbg_printf("package: 0x%x\n\r", buffer[4]);
		if ((recv_buf[4] | ((recv_buf[5] << 8) & 0xff00) | ((recv_buf[6] << 16) & 0xff0000) | ((recv_buf[7] << 24) & 0xff000000)) == (PacketNumber + 1))
			break;
		end_time = clock();
		/* CLOCKS_PER_SEC is defined at time.h */
		if ((end_time - start_time) > Time_Out_Value)
		{
			printf("Time out\n\r");
			break;
		}
	}
	printf("mode: 0x%x\n\r", (recv_buf[8] | ((recv_buf[9] << 8) & 0xff00) | ((recv_buf[10] << 16) & 0xff0000) | ((recv_buf[11] << 24) & 0xff000000)));
	if ((recv_buf[8] | ((recv_buf[9] << 8) & 0xff00) | ((recv_buf[10] << 16) & 0xff0000) | ((recv_buf[11] << 24) & 0xff000000)) == 2)
	{
		printf("The MCU run in LDROM\n\r");
	}
	else
	{
		printf("run in APROM");
	}
	printf("\n\r");
	PacketNumber += 2;
}

int CmdUpdateAprom(char *filename)
{
	int Result = 1;
	unsigned int i, j;
	unsigned long cmdData, startaddr;
	unsigned short get_cksum;
	// unsigned char Buff[256];
	// unsigned int s1,cnt;
	// for uart auto detect;
	FILE *fp;

	// if ((fp = fopen("//home//pi//aprom.bin", "rb")) == NULL)
	if((fp = fopen(filename, "rb")) == NULL)
	{
		printf("APROM FILE OPEN FALSE\n\r");
		Result = 0;
		return Result;
	}
	// get file size
	fseek(fp, 0, SEEK_END);
	file_totallen = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	// Calculate file checksum
	file_checksum = 0;
	for (i = 0; i < file_totallen; i++)
	{
		fread(&send_buf[0], sizeof(char), 1, fp);
		file_checksum = file_checksum + send_buf[0];
	}
	printf("file size=%d\n\r", file_totallen);
	printf("file checksum=0x%x\n\r", file_checksum & 0xffff);
	fseek(fp, 0, SEEK_SET);

	memset(send_buf, 0, Package_Size);
	cmdData = CMD_UPDATE_APROM;  // CMD_UPDATE_APROM
	WordsCpy(send_buf + 0, &cmdData, 4);
	WordsCpy(send_buf + 4, &PacketNumber, 4);
	PacketNumber++;
	// start address
	startaddr = 0;
	WordsCpy(send_buf + 8, &startaddr, 4);
	WordsCpy(send_buf + 12, &file_totallen, 4);

	fread(&send_buf[16], sizeof(char), 48, fp);
	usleep(1000000);
	// send CMD_UPDATE_APROM
	Result = SendData();
	if (Result == 0)
		goto out1;

	// sleep(1); //for erase time, sleep 1 sec
	// Result = RcvData();
	Result = RcvData_without_timeout();
	if(Result == 0)
	{
		printf("erase timeout!!");
		goto out1;
	}
#if 1
	for (i = 48; i < file_totallen; i = i + 56)
	{

		printf("Process=%.2f \r", (float)((float)i / (float)file_totallen) * 100);

		// clear buffer
		for(j = 0 ; j < 64 ; j++)
		{
			send_buf[j] = 0;
		}

		WordsCpy(send_buf + 4, &PacketNumber, 4);
		PacketNumber++;
		if ((file_totallen - i) > 56)
		{
			// f_read(&file1, &sendbuf[8], 56, &s1);
			fread(&send_buf[8], sizeof(char), 56, fp);
			usleep(50000);
			// read check  package
			Result = SendData();
			if (Result == 0)
				goto out1;
			Result = RcvData();
			if (Result == 0)
				goto out1;
		}
		else
		{

			fread(&send_buf[8], sizeof(char), file_totallen - i, fp);
			usleep(50000);
			// read target chip checksum
			Result = SendData();
			if (Result == 0)
				goto out1;
			Result = RcvData();
			if (Result == 0)
				goto out1;
			/*
			WordsCpy(&get_cksum, recv_buf + 8, 2);
			if ((file_checksum & 0xffff) != get_cksum)	
			{			 
				Result = 0;
				return Result;
			}
			*/
		}
	}
#endif
out1:
	return Result;
}
void check_endian(void)
{
	unsigned int a = 0x12347890;
	unsigned char *p = (unsigned char *)&a;
	printf("a=0x%x\n\r", a);
	printf("p=0x%x\n\r", *p);
	if (*p == 0x90) 
		printf("Little endian \n\r");
	if (*p == 0x12) 
		printf("Big endian \n\r");
}

int main(int argc, char *argv[])
{

#if 0
	getOsInfo();
	getCpuInfo();
	getMemoryInfo();
	check_endian();
#endif
	PacketNumber = 1; 				  // Initialize packet number
	com_handler = open_port(argv[1]);  // Open the specified COM port
	if(com_handler < 0)
	{
		perror("open_port error!\n");  // Print error message if port opening fails
		return - 1;
	}
	printf("the com port open!!\n\r");  // Print success message for port opening

	set_opt(com_handler);  // Set port options

	CHECK_UART_LINK();  // Check UART link status
	usleep(50000); 	   // Wait for 50 milliseconds

	if(SN_PACKAGE_UART() == RES_SN_OK)
		printf("connect 1\n\r");  // Print success message for first connection

	usleep(50000);
	if (SN_PACKAGE_UART() == RES_SN_OK)
		printf("connect 2 \n\r");  // Print success message for second connection

	usleep(50000);
	printf("FW version:0x%x\n\r", READFW_VERSION_UART());  // Read and print firmware version

	if(READ_PID_UART() == RES_FALSE)
	{
		printf("CHIP NO FOUND\n\r");  // Print error message if chip is not found
		goto EXIT;
	}

	usleep(50000);
	READ_CONFIG_UART();  // Read configuration from UART
	usleep(50000);

	if (CmdUpdateAprom(argv[2]) == 1)
	{
		printf("Process=%.2f \r", 100.0);  // Print progress information
		printf("programmer pass\n\r"); 	  // Print success message for programming
	}
	else
	{
		printf("programmer flase\n\r");  // Print error message for programming failure
		goto EXIT;
	}

	usleep(50000);
	RUN_TO_APROM_UART();  // Jump to APROM and run

EXIT :
	close(com_handler);  // Close the COM port
	return 0;
}