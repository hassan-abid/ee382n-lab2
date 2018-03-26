/*
 *  dm.c
 *
 *  author: Mark McDermott 
 *  Created: Feb 12, 2012
 *
 *  Simple utility to allow the use of the /dev/mem device to display memory
 *  and write memory addresses on the i.MX21.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "MemoryAccess.h"
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#define MAP_SIZE 4096UL
#define MAP_MASK (MAP_SIZE - 1)

//int main(int argc, char * argv[]) {
uint32_t dm(uint32_t target_addr){
	
	int fd = open("/dev/mem", O_RDWR|O_SYNC, S_IRUSR);
	volatile unsigned int *regs, *address;

	if(fd == -1)
	{
		printf("Unable to open /dev/mem.  Ensure it exists (major=1, minor=1)\n");
		return -1;
	}	
    
    regs = (unsigned int *)mmap(NULL, MAP_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, target_addr & ~MAP_MASK);
	
    address = regs + (((target_addr) & MAP_MASK)>>2);    	
	
    int temp = close(fd);
	if(temp == -1)
	{
		printf("Unable to close /dev/ram1.  Ensure it exists (major=1, minor=1)\n");
		return -1;
	}	

	munmap(NULL, MAP_SIZE);
	
	return *address;
}

uint32_t  pm(uint32_t target_addr, uint32_t value){
	
	int fd = open("/dev/mem", O_RDWR|O_SYNC);
	volatile unsigned int *regs, *address ;
	

	if(fd == -1)
	{
		printf("Unable to open /dev/mem.  Ensure it exists (major=1, minor=1)\n");
		return -1;
	}
	
	regs = (unsigned int *)mmap(NULL, MAP_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, target_addr & ~MAP_MASK);
	  
	address = regs + (((target_addr) & MAP_MASK)>>2); 	
	*address = value; 						// perform write command
	int temp = close(fd);
	if(temp == -1)
	{
		printf("Unable to close /dev/mem.  Ensure it exists (major=1, minor=1)\n");
		return -1;
	}	

	munmap(NULL, MAP_SIZE);
	
	
}
