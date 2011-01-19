// VGA Font Grabber
// Copyright (c) 1997 Jonathon Fowler
// This is a DOS program originally written with Borland Turbo C++ for DOS 3.1


#include <dos.h>
#include <stdio.h>

void main(void)
{
	int font, width, height, numchars;
	struct REGPACK r;
	FILE *fp;

	printf("VGA Font Grabber\n"
		   "Copyright (c) 1997 Jonathon Fowler\n");

	do {
		printf("\nSelect which font to grab:\n"
			   "  1.   8-by-8 ROM\n"
			   "  2.   8-by-14 ROM\n"
			   "  3.   8-by-16 ROM\n"
			   "  4.   9-by-16 ROM\n"
			   "  5.   9-by-14 ROM\n"
			   "  6.   Quit\n"
			   " > ");
		scanf("%d",&font);

		switch (font) {
			case 1:
				printf("Getting 8-by-8 ROM font...");

				if ((fp = fopen("88vga.dat", "wb")) != NULL) {
					width = 8;
					height = 8;
					numchars = 256;

					r.r_ax = 0x1130;	// locate the font (1st half)
					r.r_bx = 0x0300;
					intr(0x10, &r);

					fwrite(MK_FP(r.r_es, r.r_bp), 1, (8 * 128), fp);

					r.r_ax = 0x1130;	// locate the font (2nd half)
					r.r_bx = 0x0400;
					intr(0x10, &r);

					fwrite(MK_FP(r.r_es, r.r_bp), 1, (8 * 128), fp);

					fclose(fp);
				}

				printf("Done\n");
				break;
			case 2:
				printf("Getting 8-by-14 ROM font...");

				if ((fp = fopen("814vga.dat", "wb")) != NULL) {
					width = 8;
					height = 14;
					numchars = 256;

					r.r_ax = 0x1130;	// locate the font
					r.r_bx = 0x0200;
					intr(0x10, &r);

					fwrite(MK_FP(r.r_es, r.r_bp), 1, (14 * 256), fp);

					fclose(fp);
				}

				printf("Done\n");
				break;
			case 3:
				printf("Getting 8-by-16 ROM font...");

				if ((fp = fopen("816vga.dat", "wb")) != NULL) {
					width = 8;
					height = 16;
					numchars = 256;

					r.r_ax = 0x1130;	// locate the font
					r.r_bx = 0x0600;
					intr(0x10, &r);

					fwrite(MK_FP(r.r_es, r.r_bp), 1, (16 * 256), fp);

					fclose(fp);
				}

				printf("Done\n");
				break;
			case 4:
				printf("Getting 9-by-16 ROM font...");

				if ((fp = fopen("916vga.dat", "wb")) != NULL) {
					width = 9;
					height = 16;
					numchars = 256;

					r.r_ax = 0x1130;	// locate the font
					r.r_bx = 0x0700;
					intr(0x10, &r);

					fwrite(MK_FP(r.r_es, r.r_bp), 1, (16 * 256) *2, fp);

					fclose(fp);
				}

				printf("Done\n");
				break;
			case 5:
				printf("Getting 9-by-14 ROM font...");

				if ((fp = fopen("914vga.dat", "wb")) != NULL) {
					width = 9;
					height = 16;
					numchars = 256;

					r.r_ax = 0x1130;	// locate the font
					r.r_bx = 0x0500;
					intr(0x10, &r);

					fwrite(MK_FP(r.r_es, r.r_bp), 1, (14 * 256)* 2, fp);

					fclose(fp);
				}

				printf("Done\n");
				break;
			case 6:
				break;
			default:
				printf("Please try again\n");
				break;
		}
	} while (font != 6);

}
