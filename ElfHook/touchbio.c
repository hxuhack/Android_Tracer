#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <android/log.h>
#include <elf.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <linux/binder.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <poll.h>
#include <netinet/in.h>
#include <linux/input.h>
#include <sys/time.h>

#define LOG_TAG "TOUCHBIO"
#define LOGD(fmt, args...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, fmt, ##args)
#define LIBUI_PATH  "/system/lib/libui.so"
#define LIBINPUT_PATH  "/system/lib/libinput.so"
#define LIBBINDER_PATH  "/system/lib/libbinder.so"

#define ABS_MT_TOUCH_MAJOR 0x30 /* Major axis of touching ellipse */
#define ABS_MT_TOUCH_MINOR 0x31 /* Minor axis (omit if circular) */
#define ABS_MT_WIDTH_MAJOR 0x32 /* Major axis of approaching ellipse */
#define ABS_MT_WIDTH_MINOR 0x33 /* Minor axis (omit if circular) */
#define ABS_MT_ORIENTATION 0x34 /* Ellipse orientation */
#define ABS_MT_POSITION_X 0x35 /* Center X ellipse position */
#define ABS_MT_POSITION_Y 0x36 /* Center Y ellipse position */
#define ABS_MT_TOOL_TYPE 0x37 /* Type of touching device */
#define ABS_MT_BLOB_ID 0x38 /* Group a set of packets as a blob */
#define ABS_MT_TRACKING_ID 0x39 /* Unique ID of initiated contact */
#define ABS_MT_PRESSURE 0x3a /* Pressure on contact area */

//static int socketFD = -1;

int (*real_read) (int handle, void *buf, int nbyte) = 0;

char* getTouchCode(int code){
	if(code == ABS_MT_TRACKING_ID){
		return "ABS_MT_TRACKING_ID";
	}
	if(code == ABS_MT_TOUCH_MAJOR){
		return "ABS_MT_TOUCH_MAJOR";
	}
	if(code == ABS_MT_TOUCH_MINOR){
		return "ABS_MT_TOUCH_MINOR";
	}
	if(code == ABS_MT_WIDTH_MAJOR){
		return "ABS_MT_WIDTH_MAJOR";
	}
	if(code == ABS_MT_WIDTH_MINOR){
		return "ABS_MT_WIDTH_MINOR";
	}
	if(code == ABS_MT_POSITION_X){
		return "ABS_MT_POSITION_X";
	}
	if(code == ABS_MT_POSITION_Y){
		return "ABS_MT_POSITION_Y";
	}
	if(code == ABS_MT_PRESSURE){
		return "ABS_MT_PRESSURE";
	}
	return "NA";
}
int new_read (int handle, void *buf, int nbyte)
{
	LOGD("read called=================================");
	int ret = -1;
	struct input_event *pointer;
	if (nbyte > 0){

		int count = nbyte / sizeof(struct input_event);
		//struct input_event event[count] = input_event;

		int i;
		/*
		 * #define EV_ABS 0x03 //Ÿø¶Ô×ø±ê
		 * #define EV_KEY 0x01 //°ŽŒü
		 */
		for (i = 0; i < nbyte; i = i + sizeof(struct input_event)) {
			pointer = (struct input_event *)(buf+i);
			if (pointer->type == EV_ABS){

				if(strcmp(getTouchCode(pointer->code),"NA")!= 0){
					LOGD("TOUCH EVENT: time=%d.%06d, code=%s, value=%d",
							(int) pointer->time.tv_sec,
							(int) pointer->time.tv_usec,
							//pointer->type,
							getTouchCode(pointer->code),
							pointer->value);
				}
			}
		}
	}

	ret = (*real_read)(handle, buf, nbyte);
	return ret;
}
/*
int initSocket()
{
	char *server="hk.cuhk.revealer";
	struct sockaddr_un socketAddr;
	int result;
	int len;

	socketFD = socket(AF_UNIX, SOCK_STREAM, 0);
	LOGD("Socket Create Success!");
	if(-1 == socketFD)
	{
		LOGD("socket fail");
		return -1;
	}

	socketAddr.sun_family = AF_UNIX;
	socketAddr.sun_path[0] = '\0';
	strcpy(socketAddr.sun_path + 1, server);
	len = 1 + strlen(server) + offsetof(struct sockaddr_un, sun_path);
	result = connect(socketFD, (struct sockaddr*) &socketAddr, len);
	if(result == -1)
	{
		LOGD("connect fail!");
		return -1;
	}
	LOGD("Socket Connect Success!");
	return socketFD;
}


void socksend(char* sendData){
	if(socketFD > -1){
		int ret = write(socketFD, sendData, strlen(sendData));
		//LOGD("ret = %d, socketFD = %d, Data = %s", ret, socketFD, sendData);
		if (ret < 0){
			socketFD = initSocket();
		}
	}
	else{
		LOGD("socket error");
		socketFD = initSocket();
	}
}
 */
/*
 * »ñÈ¡soÎÄŒþÔÚÄÚŽæ¿ÕŒäµÄÆðÊŒµØÖ·
 */
void* get_module_base(pid_t pid, const char* module_name)
{
	FILE *fp;
	long addr = 0;
	char *pch;
	char filename[32];
	char line[1024];

	if (pid < 0) {
		/* self process */
		snprintf(filename, sizeof(filename), "/proc/self/maps", pid);
	} else {
		snprintf(filename, sizeof(filename), "/proc/%d/maps", pid);
	}

	fp = fopen(filename, "r");

	if (fp != NULL) {
		while (fgets(line, sizeof(line), fp)) {
			if (strstr(line, module_name)) {
				pch = strtok( line, "-" );
				addr = strtoul( pch, NULL, 16 );

				if (addr == 0x8000)
					addr = 0;

				break;
			}
		}

		fclose(fp) ;
	}

	return (void *)addr;
}

int hook_func(void* func, void** real_func, void* new_func, char* libpath)
{
	*real_func = func;
	LOGD("real_func = %p\n", *real_func);
	void * base_addr = get_module_base(getpid(), libpath);
	LOGD("libpath = %s, address = %p\n", libpath, base_addr);

	int fd;
	fd = open(libpath, O_RDONLY);
	if (-1 == fd) {
		LOGD("error\n");
		return -1;
	}

	Elf32_Ehdr ehdr;
	read(fd, &ehdr, sizeof(Elf32_Ehdr));

	unsigned long shdr_addr = ehdr.e_shoff;
	int shnum = ehdr.e_shnum;
	int shent_size = ehdr.e_shentsize;
	unsigned long stridx = ehdr.e_shstrndx;

	Elf32_Shdr shdr;
	lseek(fd, shdr_addr + stridx * shent_size, SEEK_SET);
	read(fd, &shdr, shent_size);

	char * string_table = (char *)malloc(shdr.sh_size);
	lseek(fd, shdr.sh_offset, SEEK_SET);
	read(fd, string_table, shdr.sh_size);
	lseek(fd, shdr_addr, SEEK_SET);

	int i;
	uint32_t out_addr = 0;
	uint32_t out_size = 0;
	uint32_t got_item = 0;
	int32_t got_found = 0;

	for (i = 0; i < shnum; i++) {
		read(fd, &shdr, shent_size);
		if (shdr.sh_type == SHT_PROGBITS) {
			int name_idx = shdr.sh_name;
			LOGD("String Tale: %s\n", &(string_table[name_idx]), sizeof(string_table));
			if (strcmp(&(string_table[name_idx]), ".got.plt") == 0
					|| strcmp(&(string_table[name_idx]), ".got") == 0) {
				out_addr = base_addr + shdr.sh_addr;
				out_size = shdr.sh_size;
				LOGD("out_addr = %p, out_size = %lx\n", out_addr, out_size);

				uint32_t toadr = (uint32_t) base_addr + shdr.sh_addr;
				LOGD("toadr = %p, out_size = %lx\n", toadr, out_size);

				for (i = 0; i < out_size; i += 4) {
					got_item = *(uint32_t *)(out_addr + i);
					LOGD("CURRENT ADDR = %x", got_item);
					if (got_item  == (uint32_t) *real_func) {
						LOGD("Found func \n");
						got_found = 1;

						uint32_t page_size = getpagesize();
						uint32_t entry_page_start = (out_addr + i) & (~(page_size - 1));
						mprotect((uint32_t *)entry_page_start, page_size, PROT_READ | PROT_WRITE);
						*(uint32_t *)(out_addr + i) = (uint32_t) new_func;

						break;
					} else if (got_item == (uint32_t) new_func) {
						LOGD("Already hooked\n");
						break;
					}
				}
				if (got_found)
					break;
			}
		}
	}

	free(string_table);
	close(fd);
	return 1;
}



/*
 * soÎÄŒþÈë¿Ú
 */
int hook_entry(char * a){
	LOGD("Hook success\n");
	LOGD("Start hooking hook_IBinder\n");


	/*
	 * libinput.so
	 * size_t EventHub::getEvents(int timeoutMillis, RawEvent* buffer, size_t bufferSize)
	 *
	 */
	hook_func(read, &real_read, new_read, LIBINPUT_PATH);

	return 0;
}