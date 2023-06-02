
#include <stdio.h>		/* Standard input/output definitions */
#include <stdlib.h>
#include <string.h>		/* String function definitions */
#include <unistd.h>		/* UNIX standard function definitions */
#include <fcntl.h>		/* File control definitions */
#include <errno.h>		/* Error number definitions */
#include     <termios.h>    /*PPSIX ²×ºÝ±±¨î©w¸q*/
#include     <sys/types.h>  
#include     <sys/stat.h>   
#include <time.h>

//#define dbg_printf printf
#define dbg_printf(...) 
unsigned char send_buf[64];
unsigned char recv_buf[64];
#define Package_Size 64
unsigned int PacketNumber;
#define Time_Out_Value 1000
int com_handler;

//open  com port
int open_port(char *comport)
{
	int fd, fset;
	int save_flag;
	//fd = open("/dev/ttyUSB0", O_RDWR);
	fd = open(comport, O_RDWR);
	fset = fcntl(fd, F_SETFL, 0);
	if (fset < 0)
		printf("fcntl failed!\n");
	else
		printf("fcntl = %d\n", fset);

	if (isatty(STDIN_FILENO) == 0)
		printf("standard input is not a terminal device\n");
	else
		printf("isatty success!\n");

	printf("fd = %d\n", fd);

	return fd;
}

//write com port
int write_port(int fd, unsigned char *buf, size_t len)
{
	int num;

	num = write(fd, buf, len);
	if (num < 0)
		printf("write failed! (%s)\n", strerror(errno));
	else

		return num;
}
//read port
int read_port(int fd, unsigned char *buf, size_t len, struct timeval *tout)
{
	fd_set inputs;
	int num, num1, ret, i, j;
	unsigned char tempbuffer[64];
	num = 0;	
	j = 0;
	FD_ZERO(&inputs);
	FD_SET(fd, &inputs);

	ret = select(fd + 1, &inputs, (fd_set *)NULL, (fd_set *)NULL, tout);
	//printf("select = %d\n", ret);
	if(ret < 0) {
		perror("select error!!");
		return ret;
	}
	if (ret > 0) {
		if (FD_ISSET(fd, &inputs)) {
			num = read(fd, buf, len);
			//printf("num t1:%d\n\r", num);
			//num1 = read(fd, tempbuffer, 64 - num);
			//printf("num t2:%d\n\r", num1);
		}
	}

	return num;
}

typedef enum {
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
	if (send_num != 64) {
		printf("Writ flase %d bytes\n\r", send_num);
		
	}	
	start_time = clock(); /* mircosecond */ 
	while (1)
	{
		recv_num = read_port(com_handler, recv_buf, Package_Size, &tout);
		if (recv_num < 0)
			printf("read failed! (%s)\n", strerror(errno));
		if (recv_num != 64) {
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
	
	//start_time = clock(); /* mircosecond */ 
	while(1)
	{
		usleep(50000); 
		send_num = write_port(com_handler, cmd, Package_Size);
		if (send_num != 64) {
			printf("Writ flase %d bytes\n\r", send_num);
		
		}	
		recv_num = read_port(com_handler, recv_buf, Package_Size, &tout);
		if (recv_num < 0)
			printf("read failed! (%s)\n", strerror(errno));
		if (recv_num != 64) {
			dbg_printf("read flase %d bytes\n\r", recv_num);
		
		}
		printf("package: 0x%x\n\r", i++);
		if ((recv_buf[4] | ((recv_buf[5] << 8) & 0xff00) | ((recv_buf[6] << 16) & 0xff0000) | ((recv_buf[7] << 24) & 0xff000000)) == (PacketNumber + 1))
			break;
		//end_time = clock(); 
		/* CLOCKS_PER_SEC is defined at time.h */ 
		//if ((end_time - start_time) > Time_Out_Value)
			//return RES_TIME_OUT;
	}
	PacketNumber += 2;
	return RES_SN_OK;
}


unsigned int READFW_VERSION_UART(void)
{
	struct timeval tout;
	clock_t start_time, end_time;
	float total_time = 0;
	int send_num, recv_num; 
	tout.tv_sec = 1;
	tout.tv_usec = 100 * 1000;
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
	if (send_num != 64) {
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
			return 0;

	}
	dbg_printf("fw version:0x%x\n\r", recv_buf[8]);
	dbg_printf("\n\r");
	PacketNumber += 2;
	return (recv_buf[8] | ((recv_buf[9] << 8) & 0xff00) | ((recv_buf[10] << 16) & 0xff0000) | ((recv_buf[11] << 24) & 0xff000000));
}



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
	if (send_num != 64) {
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
#if 0
	PacketNumber += 2;
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
	if (send_num != 64) {
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
	if (send_num != 64) {
		printf("Writ fale %d bytes\n\r", send_num);
		
	}					
	PacketNumber += 2;	
}


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
	if (send_num != 64) {
		printf("Writ fale %d bytes\n\r", send_num);
		
	}					
	PacketNumber += 2;	
}
int mode;
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
	if (send_num != 64) {
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
#define CMD_UPDATE_APROM	0x000000A0

unsigned short Checksum(unsigned char *buf, int len)
{
	int i;
	unsigned short c;

	for (c = 0, i = 0; i < len; i++) {
		c += buf[i];
	}
	return (c);
}

void WordsCpy(void *dest, void *src, unsigned int size)
{
	unsigned char *pu8Src, *pu8Dest;
	unsigned int i;
    
	pu8Dest = (unsigned char *)dest;
	pu8Src  = (unsigned char *)src;
    
	for (i = 0; i < size; i++)
		pu8Dest[i] = pu8Src[i]; 
}
unsigned short gcksum;
int SendData(void)
{
	int Result;
	int send_num; 
	gcksum = Checksum(send_buf, Package_Size);

	send_num = write_port(com_handler, send_buf, Package_Size);
	if (send_num != 64) {
		printf("Writ fale %d bytes\n\r", send_num);
		return 0;
	}				
	return 1;
}

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

	//if (inpw(pBuf) != PacketNumber)
	//{
	//	dbg_printf("g_packno=%d rcv %d\n", g_packno, inpw(pBuf));
	//	Result = 0;
	//}
	//else
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
		//end_time = clock(); 
		/* CLOCKS_PER_SEC is defined at time.h */ 
		//if ((end_time - start_time) > Time_Out_Value)
		//{
		//	printf("Time out\n\r");
		//	break;
	//	}
	}
	
	pBuf = recv_buf;
	WordsCpy(&lcksum, pBuf, 2);
	pBuf += 4;

	//if (inpw(pBuf) != PacketNumber)
	//{
	//	dbg_printf("g_packno=%d rcv %d\n", g_packno, inpw(pBuf));
	//	Result = 0;
	//}
	//else
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
	if (send_num != 64) {
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
	//unsigned char Buff[256];
	//unsigned int s1,cnt;
	//for uart auto detect;
	FILE *fp;
		
	//if ((fp = fopen("//home//pi//aprom.bin", "rb")) == NULL)
	if((fp = fopen(filename, "rb")) == NULL)
	{
		printf("APROM FILE OPEN FALSE\n\r");
		Result = 0;
		return Result;
	}	
	//get file size
	fseek(fp, 0, SEEK_END);
	file_totallen = ftell(fp);
	fseek(fp, 0, SEEK_SET);
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
	cmdData = CMD_UPDATE_APROM;  //CMD_UPDATE_APROM
	WordsCpy(send_buf + 0, &cmdData, 4);
	WordsCpy(send_buf + 4, &PacketNumber, 4);
	PacketNumber++;
	//start address
		startaddr = 0;
	WordsCpy(send_buf + 8, &startaddr, 4);
	WordsCpy(send_buf + 12, &file_totallen, 4);
	
	fread(&send_buf[16], sizeof(char), 48, fp);
	usleep(1000000);
	//send CMD_UPDATE_APROM
	Result = SendData();
	if (Result == 0)
		goto out1;
	

	//sleep(1); //for erase time, sleep 1 sec
	Result = RcvData();
	//Result = RcvData_without_timeout();
	if(Result == 0)
		goto out1;
#if 1
	for (i = 48; i < file_totallen; i = i + 56)
	{

	
		printf("Process=%.2f \r", (float)((float)i / (float)file_totallen) * 100);

		//clear buffer
for(j = 0 ; j < 64 ; j++)
		{
			send_buf[j] = 0;
		}

		WordsCpy(send_buf + 4, &PacketNumber, 4);
		PacketNumber++;
		if ((file_totallen - i) > 56)
		{			
			//f_read(&file1, &sendbuf[8], 56, &s1);
			fread(&send_buf[8], sizeof(char), 56, fp);
			usleep(50000);
			//read check  package
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
			//read target chip checksum
		  Result = SendData();
			if (Result == 0)
				goto out1;
			Result = RcvData();
			if (Result == 0)			
				goto out1;	
			
			WordsCpy(&get_cksum, recv_buf + 8, 2);
			if ((file_checksum & 0xffff) != get_cksum)	
			{			 
				Result = 0;
				return Result;
			}
		}
	}
#endif
out1:
	return Result;

}




int main(int argc, char *argv[])
{
	
	PacketNumber = 1;
	com_handler = open_port(argv[1]);
	if (com_handler < 0) {
		perror("open_port error!\n");
		return -1;
	}
	printf("the com port open!!\n\r");

	set_opt(com_handler);

	CHECK_UART_LINK();
	usleep(50000);

		if (SN_PACKAGE_UART() == RES_SN_OK)	
			printf("connect 1\n\r");

		usleep(50000); 
		if (SN_PACKAGE_UART() == RES_SN_OK)	
			printf("connect 2 \n\r");
	
		usleep(50000); 
		printf("FW version:0x%x\n\r", READFW_VERSION_UART());
		usleep(50000); 
		if (READ_PID_UART() == RES_FALSE)
		{			
			printf("CHIP NO FOUND\n\r");
			goto EXIT;
		}
		usleep(50000); 
		READ_CONFIG_UART();
		usleep(50000);  
	
		if (CmdUpdateAprom(argv[2]) == 1)
		{
			printf("Process=%.2f \r", 100.0);
			printf("programmer pass\n\r");
		}
		else
		{
			printf("programmer flase\n\r");
			goto EXIT;
		}
	
	
		usleep(50000); 
		RUN_TO_APROM_UART();   //mcu jump to aprom and run
	

EXIT:
	close(com_handler);
}