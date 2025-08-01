#ifdef LILOTAFS_LOCAL
#include <dirent.h>
#include "flash.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"
#include "lilotafs.h"


// NOTE: testing utility commands:
// W [fd] [length]        | write random data of length [length] to file [fd]
// w [fd] [length]        | write only 0xFF of length [length] to file [fd]
// d [fd]                 | delete file [fd]
// c [fd]                 | close file [fd]
// o [create?] [filename] | open file with name [filename], choose whether to create
// i                      | query filesystem for information


int main(int argc, char *argv[]) {
	srand(time(NULL));

	if (argc < 2) {
		printf("error: must enter arguments\n");
		return 1;
	}

	printf("total size %d, sector size %d\n", TOTAL_SIZE, SECTOR_SIZE);

	char *disk_name = argv[1];
	int disk = open(disk_name, O_RDWR);

	struct lilotafs_context ctx;
	memset(&ctx, 0, sizeof(struct lilotafs_context));

	printf("mount: %d\n", lilotafs_mount(&ctx, TOTAL_SIZE, disk));

	printf("read only: %d\n", O_RDONLY);
	printf("write only: %d\n", O_WRONLY);
	printf("write only and create: %d\n", O_WRONLY | O_CREAT);

	// lilotafs_mkdir(&ctx, "/hello", 0);
	// int file = lilotafs_open(&ctx, "hello/file.txt", 65, 0);
	// if (file == -1) {
	// 	printf("cannot open file: %d\n", lilotafs_errno(&ctx));
	// }
	// else {
	// 	lilotafs_close(&ctx, file);
	// }
	//
	// DIR *dir = lilotafs_opendir(&ctx, "sys/");
	// DIR *dir = lilotafs_opendir(&ctx, "//./");
	//
	// while (1) {
	// 	struct dirent *de = lilotafs_readdir(&ctx, dir);
	// 	if (de == NULL)
	// 		break;
	// 	printf("%s\n", de->d_name);
	// }
	//
	// lilotafs_closedir(&ctx, dir);
	//
	// lilotafs_unmount(&ctx);
	//
	// return 0;

	char input[101];
	char *token;

	char mode;
	uint32_t fd;
	uint32_t flag;
	char filename[64];
	bool success;

	printf("\n");
	while (1) {
		mode = 0, fd = 0, flag = 0, success = true;
		memset(filename, 0, 64);

		printf("> ");
		fgets(input, 100, stdin);

		for (uint32_t i = 0; i < strlen(input); i++) {
			if (input[i] == '\n') {
				input[i] = 0;
				break;
			}
		}

		token = strtok(input, " ");
		for (uint32_t i = 0; token != NULL; i++) {
			if (i == 0) {
				mode = input[0];
				if (mode == 'q') {
					lilotafs_unmount(&ctx);
					close(disk);
					return 0;
				}
				if (mode != 'o' && mode != 'l' && mode != 'w' && mode != 'd' && mode != 'c' && mode != 'i' && mode != 'W') {
					success = false;
					break;
				}
			}

			if (i == 1 && mode != 'i') {
				if (mode == 'o')
					flag = strtoul(token, NULL, 0);
				else
					fd = strtoul(token, NULL, 0);
			}

			if (i == 2 && mode != 'o' && mode != 'c' && mode != 'i')
				flag = strtoul(token, NULL, 0);

			if ((i >= 2 && mode == 'o') || (i >= 1 && mode == 'l')) {
				if (strlen(filename) + strlen(token) > 63) {
					success = false;
					break;
				}
				strcat(filename, token);
			}

			token = strtok(NULL, " ");
		}

		if (!success) {
			printf("error\n");
			continue;
		}

		// printf("mode = %c\n", mode);
		// printf("fd = %u\n", fd);
		// printf("flag = %u\n", flag);
		// printf("filename = %.63s\n", filename);

		if (mode == 'o') {
			int ret_fd = lilotafs_open(&ctx, filename, flag, 0);
			printf("open file %.63s fd: %d\n", filename, ret_fd);
			if (ret_fd == -1)
				printf("error: %d\n", lilotafs_errno(&ctx));
		}
		else if (mode == 'c') {
			int result = lilotafs_close(&ctx, fd);
			printf("close fd %d: %d\n", fd, result);
		}
		else if (mode == 'd') {
			// int result = lilotafs_delete(&ctx, fd);
			// printf("delete fd %d: %d\n", fd, result);

		}
		else if (mode == 'w' || mode == 'W') {
			uint32_t size = flag;
			uint8_t *data = (uint8_t *) malloc(size);
			for (uint32_t i = 0; i < size; i++)
				data[i] = mode == 'W' ? RANDOM_NUMBER(0, 255) : 0xFF;
			uint32_t result = lilotafs_write(&ctx, fd, data, size);
			printf("write %u bytes to fd %d: %d\n", size, fd, result);
			free(data);
		}
		else if (mode == 'i') {
			uint32_t count = lilotafs_count_files(&ctx);
			uint32_t largest_file = lilotafs_get_largest_file_size(&ctx);
			uint32_t head = lilotafs_get_head(&ctx);
			uint32_t tail = lilotafs_get_tail(&ctx);
			printf("number of files: %u\n", count);
			printf("largest file: %u\n", largest_file);
			printf("head: 0x%x\n", head);
			printf("tail: 0x%x\n", tail);
		}
		else if (mode == 'l') {
			DIR *dir = lilotafs_opendir(&ctx, filename);
			if (dir == NULL) {
				printf("open dir failed, %d\n", lilotafs_errno(&ctx));
				continue;
			}

			while (true) {
				struct dirent *de = lilotafs_readdir(&ctx, dir);
				if (de == NULL)
					break;
				printf("%.63s (%s)\n", de->d_name, de->d_type == DT_DIR ? "dir" : "reg");
			}

			lilotafs_closedir(&ctx, dir);
		}
	}

	
	// uint32_t fd = vfs_open("lilota", LILOTAFS_WRITABLE | LILOTAFS_READABLE);
	// uint32_t fd = vfs_open("test", LILOTAFS_WRITABLE | LILOTAFS_READABLE | LILOTAFS_CREATE);
	// printf("fd: %d\n", fd);
	// if (fd == UINT32_MAX) {
	// 	close(disk);
	// 	return 1;
	// }
	//
	// printf("largest file size: %d\n", vfs_get_largest_file_size());

	// write test
	// char *content = "lilota FS test";
	// uint32_t result = vfs_write(fd, content, strlen(content));
	// printf("write result: %d\n", result);

	// char *content2 = "Lorem ipsum dolor sit amet, consectetur adipiscing elit";
	// result = vfs_write(fd, content2, strlen(content2));
	// printf("%d\n", result);

	// char *content2 = (char *) malloc(301);
	// result = vfs_write(fd, content2, 301);
	// printf("%d\n", result);

	// read test
	// uint32_t file_size = vfs_get_size(fd);
	// printf("size: %d\n", file_size);
	// char *buffer = (char *) malloc(file_size);
	// vfs_read(fd, buffer, 2, file_size - 5);
	// for (uint32_t i = 0; i < file_size - 5; i++)
	// 	printf("%c", buffer[i]);
	// printf("\n");
	// free(buffer);

	// delete test
	// uint32_t delete_result = vfs_delete(fd);
	// printf("delete: %d\n", delete_result);
	//
	// printf("close: %d\n", vfs_close(fd));
	
	// uint8_t buffer[4];
	// flash_read(buffer, 0x118, 4);

	// for (int i = 0; i < 4; i++)
	// 	printf("%02X\n", buffer[i]);

	close(disk);

	return 0;
}
#endif
