#include "types.h"
#include "utils.h"
#include "lib.h"

#define LEN 6000

union DirEntry {
	uint8_t byte[128];
	struct {
		int32_t inode;
		char name[64];
	};
};
typedef union DirEntry DirEntry;

int ls(char *destFilePath) {
	// STEP 8
    // TODO: ls
    /* output format
    ls /
    boot dev usr
    */

	printf("ls %s, ", destFilePath);
	int fd = open(destFilePath, O_READ);

	printf("ls fd: %d\n", fd);
	if (fd == -1)
	{
		return 0;
	}

	DirEntry direntry;
	int read_num = 0;
	read_num = read(fd, (uint8_t *)&direntry, 128);
	
	while (read_num > 0)
	{
		//i++;
		//printf("read_num = %d\n", read_num);
		if (direntry.inode != 0)
		{
			printf("%s ", direntry.name);
		}
		read_num = read(fd, (uint8_t *)&direntry, 128);
	}
	printf("\n");
	close(fd);

	return 0;
}

int cat(char *destFilePath) {
	// STEP 9
    // TODO: cat
    /* output format
    cat /usr/test
    ABCDEFGHIJKLMNOPQRSTUVWXYZ
    */

	char str[LEN];
	int fd = open(destFilePath, O_READ);
	printf("cat fd = %d\n", fd);

	if (fd == -1)
	{
		return 0;
	}

	int ret = read(fd, (uint8_t*)str, LEN);

	str[ret] = '\0';
	printf("%s\n", str);
	close(fd);

	return 0;
}

int uEntry(void)
{
	// STEP 2-9
	// TODO: try different test case when you finish a function

	/* filesystem test */
	int fd = 0;
	int i = 0;
	char tmp = 0;

	ls("/");
	ls("/boot/");
	ls("/dev/");
	ls("/usr/");

	printf("create /usr/test and write alphabets to it\n");
	fd = open("/usr/test", O_WRITE | O_READ | O_CREATE);
	//write(fd, (uint8_t*)"Hello, World!\n", 14);
	//write(fd, (uint8_t*)"This is a demo!\n", 16);
	//for (i = 0; i < 2049; i ++) {
	//for (i = 0; i < 2048; i ++) {
	//for (i = 0; i < 1025; i ++) {
	//for (i = 0; i < 512; i ++) {
	for (i = 0; i < 26; i++)
	{
		tmp = (char)(i % 26 + 'A');
		write(fd, (uint8_t *)&tmp, 1);
	}
	close(fd);
	ls("/usr/");
	cat("/usr/test");
	printf("\n");
	printf("rm /usr/test\n");
	remove("/usr/test");
	ls("/usr/");

	//ls("/");

	printf("rmdir /usr/\n");
	remove("/usr/");
	//remove("/dev");
	ls("/");
	//ls("/dev");
	printf("create /usr/\n");
	fd = open("/usr/", O_CREATE | O_DIRECTORY);
	close(fd);
	ls("/");
	//fd = open("/usr/test", O_WRITE | O_READ);
	//close(fd);
	//ls("/usr");
	//fd = open("/usr/test/", O_CREATE);
	//close(fd);
	//ls("/usr/");
	//fd = open("/usr/test", O_CREATE);
	//close(fd);
	//ls("/usr/");

	exit();
	/**
    Output format can be found in 
    http://114.212.80.195:8170/2019oslab/lab5/
    */
	return 0;
}

// int uEntry(void) {
//     // STEP 2-9
//     // TODO: try different test case when you finish a function
    
//     /* filesystem test */
// 	int fd = 0;
// 	int fd2;
// 	int fd3;
// 	int i = 0;
// 	char tmp = 0;
// 	//printf("ssssss\n");

// 	ls("/");
// 	//ls("/boot/");
// 	ls("/dev/");
// 	//ls("/usr/");

// 	//printf("create /usr/test and write alphabets to it\n");
// 	fd = open("/usr/test", O_WRITE | O_READ | O_CREATE);
// 	fd2 = open("/usr/test2", O_WRITE | O_READ);
// 	fd3 = open("/usr/test3", O_WRITE | O_READ | O_CREATE);

// 	printf("test result: fd: %d, fd2: %d, fd3: %d\n", fd, fd2, fd3);

// 	//write(fd, (uint8_t*)"Hello, World!\n", 14);
// 	//write(fd, (uint8_t*)"This is a demo!\n", 16);
// 	//for (i = 0; i < 2049; i ++) {
// 	//for (i = 0; i < 2048; i ++) {
// 	//for (i = 0; i < 1025; i ++) {
// 	//for (i = 0; i < 512; i ++) {
// 	for (i = 0; i < 52; i ++) {
// 		tmp = (char)(i % 26 + 'A');
// 		write(fd, (uint8_t*)&tmp, 1);
// 	}
// 	close(fd);
// 	close(fd3);

// 	ls("/usr/");
// 	cat("/usr/test");  //print file content on the terminal
// 	cat("/usr/test2");

// 	printf("\n");
// 	printf("rm /usr/test\n");
	
// 	int rem = remove("/usr/test");
// 	printf("rem = %d\n", rem);

// 	ls("/usr/");
// 	//printf("rmdir /usr/\n");
// 	//remove("/usr/");
// 	//remove("/dev");
// 	ls("/");
// 	//ls("/dev");
// 	printf("create /usr/\n");
// 	fd = open("/usr/", O_CREATE | O_DIRECTORY);
// 	printf("2nd test res: fd = %d\n", fd);
// 	int final = close(fd);
// 	ls("/");

// 	printf("final = %d\n", final);
// 	//fd = open("/usr/test", O_WRITE | O_READ);
// 	//close(fd);
// 	//ls("/usr");
// 	//fd = open("/usr/test/", O_CREATE);
// 	//close(fd);
// 	//ls("/usr/");
// 	//fd = open("/usr/test", O_CREATE);
// 	//close(fd);
// 	//ls("/usr/");

// 	exit();
//     /**
//     Output format can be found in 
//     http://114.212.80.195:8170/2019oslab/lab5/
//     */
// 	return 0;
// }


