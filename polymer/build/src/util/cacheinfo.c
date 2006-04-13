#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include "compat.h"

#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT   0x83F0
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT  0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT  0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT  0x83F3

typedef struct {
	char magic[8];	// 'Polymost'
	long xdim, ydim;	// of image, unpadded
	long flags;		// 1 = !2^x, 2 = has alpha, 4 = lzw compressed
} texcacheheader;
typedef struct {
	long size;
	long format;
	long xdim, ydim;	// of mipmap (possibly padded)
	long border, depth;
} texcachepicture;

int main(int argc, char **argv)
{
	DIR *dir;
	struct dirent *dirent;
	struct stat st;
	FILE *fp;
	texcacheheader head;
	texcachepicture mip;
	
	dir = opendir(".");
	while ((dirent = readdir(dir))) {
		if (stat(dirent->d_name, &st)) {
			printf("%s: failed to stat\n", dirent->d_name);
			continue;
		}
		if (!(st.st_mode&S_IFREG)) {
			printf("%s: not a regular file\n", dirent->d_name);
			continue;
		}
		
		fp = fopen(dirent->d_name,"rb");
		if (!fp) {
			printf("%s: failed to open\n", dirent->d_name);
			continue;
		}
		
		if (fread(&head, sizeof(head), 1, fp) != 1) {
			fclose(fp);
			printf("%s: failed to read header\n", dirent->d_name);
			continue;
		}
		head.xdim = B_LITTLE32(head.xdim);
		head.ydim = B_LITTLE32(head.ydim);
		head.flags = B_LITTLE32(head.flags);
		if (fread(&mip, sizeof(mip), 1, fp) != 1) {
			fclose(fp);
			printf("%s: failed to read mipmap header\n", dirent->d_name);
			continue;
		}
		mip.format = B_LITTLE32(mip.format);
		fclose(fp);
		if (memcmp(head.magic, "Polymost", 8)) {
			printf("%s: bad signature\n", dirent->d_name);
			continue;
		}
		else {
			char *format;
			char flags[4] = "", flagsc = 0;
			switch (mip.format) {
				case GL_COMPRESSED_RGB_S3TC_DXT1_EXT: format = "RGB DXT1"; break;
				case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT: format = "RGBA DXT1"; break;
				case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT: format = "RGBA DXT3"; break;
				case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT: format = "RGBA DXT5"; break;
				default: format = "Unknown"; break;
			}
			if (head.flags&1) flags[flagsc++] = '2';
			if (head.flags&2) flags[flagsc++] = 'A';
			if (head.flags&4) flags[flagsc++] = 'L';
			flags[flagsc++] = 0;
			
			printf("%s: flags=%s format=%s\n", dirent->d_name, flags, format);
		}
	}
	closedir(dir);
	
	return 0;
}