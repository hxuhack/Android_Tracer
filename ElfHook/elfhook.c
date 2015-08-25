#include "elfhook.h"

void parseBinderTransData(struct binder_transaction_data *tmp_buffer, int cmd)
{
	if (tmp_buffer->data_size > 0){

		int sender_pid = (int)tmp_buffer->sender_pid;//pid_t sender_pid;
		int sender_euid = (int)tmp_buffer->sender_euid;//uid_t sender_euid;
		int data_size = (int)tmp_buffer->data_size;//size_t data_size;

		unsigned int target = (unsigned int)tmp_buffer->target.ptr; //union {size_t handle; void *ptr;} target;
		unsigned int cookie = (unsigned int)tmp_buffer->cookie; //void *cookie
		unsigned int code = (unsigned int)tmp_buffer->code;//unsigned int code;
		unsigned int flags = (unsigned int)tmp_buffer->flags; //unsigned int flags
		unsigned int offsets_size = (unsigned int)tmp_buffer->offsets_size;//size_t offsets_size;
		unsigned int buffer = (unsigned int)tmp_buffer->data.ptr.buffer; //union {struct {const void *buffer;const void *offsets;} ptr; uint8_t buf[8];}
		unsigned int offsets = (unsigned int)tmp_buffer->data.ptr.offsets;
		//LOGD_C("cmd = %s, target = %p, cookie = %p, code = %p, flags = %p, sender_pid = %p, sender_euid = %p, data_size = %d, offsets_size = %p, buffer = %p, offsets = %p \n",
		//cmd, target, cookie, code, flags, sender_pid, sender_euid, data_size, offsets_size, buffer, offsets);

		char *tmpChar = (char *) malloc (data_size+1);
		uint16_t *tmpUINT16 = tmp_buffer->data.ptr.buffer;
		if (tmpUINT16 > 100000){
			int k = 0, m = 0;
			for(k = 0; k< data_size/2; k = k + 1)
			{
				if ((*(tmpUINT16+k) > 32 && *(tmpUINT16+k) < 127)){
					*(tmpChar + m) = *(tmpUINT16+k);
				}
				else{
					*(tmpChar + m) = '*';
					//LOGD_C("*%d = %x \n",m,*(tmpUINT16+m));
				}
				m++;
			}
			*(tmpChar + m) = '\0';

            char sendData[1024];
	        snprintf(sendData, sizeof(sendData), "BIPC:%s %s \r\n", GetHhmmssUsec(), tmpChar);
			sendtofiat(sendData);
			//struct flat_binder_object *flatObj = tmp_buffer->data.ptr.buffer;
		}
		free(tmpChar);
	}
}

int isBinderCommand(unsigned int *tmpValue){
	int ret;
	switch (*tmpValue){
	case BC_TRANSACTION:
		ret = 1;
		break;
	case BC_REPLY:
		ret = 2;
		break;
	default:
		ret = 0;
		break;
	}

	return ret;
}
char* isBinderRead(unsigned int *tmpValue){
	int ret;
	switch (*tmpValue){
	case BR_TRANSACTION:
		ret = 3;
		break;
	case BR_REPLY:
		ret = 4;
		break;
	default:
		ret = 0;
		break;
	}
	return ret;
}

int new_ioctl (int __fd, unsigned long int __request, void * arg)
{
	//LOGD_C("ioctl called \n");
	int ret = -1;

	if (__request == BINDER_WRITE_READ){ //arg Ϊbinder_write_read��
		struct binder_write_read *bwr = arg;
		//char* pid_name;
		int cmd;
		if(bwr->write_size > 0){
			void *tmp = bwr->write_buffer;
			int j = 0;
			for(j = 0; j< bwr->write_size; j = j+ 4){
				unsigned int *tmpValue = tmp+j;
				cmd = isBinderCommand(tmpValue);
				if (cmd > 0){
					//		LOGD_C("CMD = %s", cmd);
					//if (strcmp(cmd, "BC_TRANSACTION") == 0 || strcmp(cmd, "BC_REPLY") == 0)
					if (cmd == 1)
					{
						struct binder_transaction_data *btd;
						btd = tmp+j+4;
						parseBinderTransData(btd,cmd);
						j = j+ 10;
					}
				}
				else{
					//	LOGD_C("Parameter = %p", *tmpValue);
				}
				//free(cmd);
			}
			//LOGD_C("-------------------------------------------");
		}
		if(bwr->read_size > 0){
			void *tmp = bwr->read_buffer;
			int j = 0;
			for(j = 0; j< bwr->read_size; j = j+ 4){
				unsigned int *tmpValue = tmp+j;
				cmd = isBinderRead(tmpValue);
				if (cmd > 0){
					//LOGD_C("CMD = %s", cmd);
					//if (strcmp(cmd, "BR_TRANSACTION") == 0 || strcmp(cmd, "BR_REPLY") == 0)
					if (cmd == 3)
					{
						struct binder_transaction_data *btd;
						btd = tmp+j+4;
						parseBinderTransData(btd,cmd);
						j = j+ 10;
					}
				}
				else{
					//LOGD_C("Parameter = %p", *tmpValue);
				}
				//free(cmd);
			}
		}
		//LOGD_C("==============================================");
	}

	ret = (*real_ioctl)(__fd, __request, arg);
	return ret;
}

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
	LOGD_C("real_func = %p\n", *real_func);
	void * base_addr = get_module_base(getpid(), libpath);
	LOGD_C("libpath = %s, address = %p\n", libpath, base_addr);

	int fd;
	fd = open(libpath, O_RDONLY);
	if (-1 == fd) {
		LOGD_C("error\n");
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
			LOGD_C("String Tale: %s\n", &(string_table[name_idx]), sizeof(string_table));
			if (strcmp(&(string_table[name_idx]), ".got.plt") == 0
					|| strcmp(&(string_table[name_idx]), ".got") == 0) {
				out_addr = base_addr + shdr.sh_addr;
				out_size = shdr.sh_size;
				LOGD_C("out_addr = %p, out_size = %lx\n", out_addr, out_size);

				uint32_t toadr = (uint32_t) base_addr + shdr.sh_addr;
				LOGD_C("toadr = %p, out_size = %lx\n", toadr, out_size);

				for (i = 0; i < out_size; i += 4) {
					got_item = *(uint32_t *)(out_addr + i);
					//LOGD_C("got_item = %p\n, real_ioctl= %p\n", got_item, real_ioctl);
					if (got_item  == (uint32_t) *real_func) {
						LOGD_C("Found func \n");
						got_found = 1;

						uint32_t page_size = getpagesize();
						uint32_t entry_page_start = (out_addr + i) & (~(page_size - 1));
						mprotect((uint32_t *)entry_page_start, page_size, PROT_READ | PROT_WRITE);
						*(uint32_t *)(out_addr + i) = (uint32_t) new_func;

						break;
					} else if (got_item == (uint32_t) new_func) {
						LOGD_C("Already hooked\n");
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
