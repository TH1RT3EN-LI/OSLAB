/*
 * operations on IDE disk.
 */

#include "fs.h"
#include "lib.h"
#include <mmu.h>

// Overview:
// 	read data from IDE disk. First issue a read request through
// 	disk register and then copy data from disk buffer
// 	(512 bytes, a sector) to destination array.
//
// Parameters:
//	diskno: disk number.
// 	secno: start sector number.
// 	dst: destination for data read from IDE disk.
// 	nsecs: the number of sectors to read.
//
// Post-Condition:
// 	If error occurred during read the IDE disk, panic. 
// 	
// Hint: use syscalls to access device registers and buffers
static void ide_transfer(u_int diskno, u_int secno, void *buf, u_int nsecs, int is_write) {
	const u_int SECTOR_SIZE = 0x200;
	const u_int DEV_ADDR = 0x13000000;
	u_int offset_begin = secno * SECTOR_SIZE;
	u_int offset_end = offset_begin + nsecs * SECTOR_SIZE;
	u_int offset = 0;
	u_char status = 0;
	u_char rw_value = is_write ? 1 : 0;

	while (offset_begin + offset < offset_end) {
		u_int now_offset = offset_begin + offset;
		if (syscall_write_dev((u_int)&diskno, DEV_ADDR + 0x10, 4) < 0)
			user_panic("ide_transfer: diskno write error!");
		if (syscall_write_dev((u_int)&now_offset, DEV_ADDR + 0x0, 4) < 0)
			user_panic("ide_transfer: offset write error!");

		if (is_write) {
			if (syscall_write_dev((u_int)(buf + offset), DEV_ADDR + 0x4000, SECTOR_SIZE) < 0)
				user_panic("ide_transfer: data write error!");
		}

		if (syscall_write_dev((u_int)&rw_value, DEV_ADDR + 0x20, 1) < 0)
			user_panic("ide_transfer: rw_value write error!");

		status = 0;
		if (syscall_read_dev((u_int)&status, DEV_ADDR + 0x30, 1) < 0)
			user_panic("ide_transfer: status read error!");
		if (status == 0)
			user_panic(is_write ? "ide write failed!" : "ide read failed!");

		if (!is_write) {
			if (syscall_read_dev((u_int)(buf + offset), DEV_ADDR + 0x4000, SECTOR_SIZE) < 0)
				user_panic("ide_transfer: data read error!");
		}

		offset += SECTOR_SIZE;
	}
}

void ide_read(u_int diskno, u_int secno, void *dst, u_int nsecs) {
	ide_transfer(diskno, secno, dst, nsecs, 0);
}

void ide_write(u_int diskno, u_int secno, void *src, u_int nsecs) {
	ide_transfer(diskno, secno, src, nsecs, 1);
}
