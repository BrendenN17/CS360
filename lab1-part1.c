/**********WORKING IMPLEMENTATION OF LAB1 PART1 REQUIREMENTS TO OPEN AND DISPLAY PARTITIONS (INCLUDING EXTENDED ONES)**********/
#include <stdio.h>
#include <fcntl.h>
#include <string.h>

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

typedef struct partition {
	u8 drive;             /* drive number FD=0, HD=0x80, etc. */

	u8  head;             /* starting head */
	u8  sector;           /* starting sector */
	u8  cylinder;         /* starting cylinder */

	u8  sys_type;         /* partition type: NTFS, LINUX, etc. */

	u8  end_head;         /* end head */
	u8  end_sector;       /* end sector */
	u8  end_cylinder;     /* end cylinder */

	u32 start_sector;     /* starting sector counting from 0 */
	u32 nr_sectors;       /* number of of sectors in partition */
}Partition;

Partition *p;
int fd;
 char buf[512];


int main()
{
partition(); //prints first 4 partitions and extended ones

}

void partition()
{
int fd = open("vdisk", O_RDONLY);//open disk for reading
if (fd < 0)
{
	printf("failed to open disk\n");
	exit(0);
}

read (fd, buf, 512);//reads mbr into buffer of 512
p = (Partition *)&buf[0x1BE];//assigning pointer to point at the start of the partitions.
printf("Device\t\tStart_Sector\t\tEnd_Sector\t\tSector size\t\tSize\t\tID\n");


for (int i = 1; i < 5; ++i)
{
printf("vdisk%d\t\t", i);
printf("%d\t\t\t", p->start_sector);//accesses start of struct
printf("%d\t\t\t", (p->start_sector + p->nr_sectors -1));//prints end sector
printf("%d\t\t\t", p->nr_sectors);
printf("%dk\t\t", (p->nr_sectors / 2));
printf("%2x\n", p->sys_type);

int tracker = p->start_sector;//tracks size of extended partitions
if (p->sys_type == 5)//extended partition
{
lseek(fd, (long)(p->start_sector*512), SEEK_SET);//seek p4 to begin extended partitions

read(fd, buf, 512); //read local mbr
p = (Partition *)&buf[0x1BE];//assigning pointer to point at the start of the partitions.
//printing partition contents
printf("vdisk5\t\t");
printf("%d\t\t\t", p->start_sector + tracker);//accesses start of struct
printf("%d\t\t\t", (p->start_sector + p->nr_sectors -1 + tracker));//prints end sector
printf("%d\t\t\t", p->nr_sectors);
printf("%dk\t\t", (p->nr_sectors / 2));
printf("%2x\n", p->sys_type);
p++;
lseek(fd, ((tracker + p->start_sector)*512), SEEK_SET);
tracker +=p->start_sector;//updating tracker to help go to next partition


read(fd, buf, 512); //read local mbr
p = (Partition *)&buf[0x1BE];//assigning pointer to point at the start of the partitions.
//printing partition contents
printf("vdisk6\t\t");
printf("%d\t\t\t", p->start_sector + tracker);//accesses start of struct
printf("%d\t\t\t", (p->start_sector + p->nr_sectors -1 + tracker));//prints end sector
printf("%d\t\t\t", p->nr_sectors);
printf("%dk\t\t", (p->nr_sectors / 2));
printf("%2x\n", p->sys_type);
p++;
lseek(fd, ((tracker + p->start_sector)*512), SEEK_SET);
tracker +=p->start_sector;//updating tracker to help go to next partition

read(fd, buf, 512); //read local mbr
p = (Partition *)&buf[0x1BE];//assigning pointer to point at the start of the partitions.
//printing partition contents
printf("vdisk7\t\t");
printf("%d\t\t\t", tracker);//accesses start of struct
printf("%d\t\t\t", (p->start_sector + p->nr_sectors -1 + tracker));//prints end sector
printf("%d\t\t\t", p->nr_sectors);
printf("%dk\t\t", (p->nr_sectors / 2));
printf("%2x\n", p->sys_type);
p++;
}

p++; //advances to the next partition in the table mbr
}
}
