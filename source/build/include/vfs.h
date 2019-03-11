
#pragma once

#ifndef vfs_h_
#define vfs_h_

#include "compat.h"

#ifdef USE_PHYSFS

#include "physfs.h"

using buildvfs_FILE = PHYSFS_File *;
#define buildvfs_EOF (-1)
#define buildvfs_fread(p, s, n, fp) PHYSFS_readBytes((fp), (p), (s)*(n))
#define buildvfs_fwrite(p, s, n, fp) PHYSFS_writeBytes((fp), (p), (s)*(n))
#define buildvfs_fopen_read(fn) PHYSFS_openRead(fn)
#define buildvfs_fopen_write(fn) PHYSFS_openWrite(fn)
#define buildvfs_fopen_write_text(fn) PHYSFS_openWrite(fn)
#define buildvfs_fopen_append(fn) PHYSFS_openAppend(fn)
static inline int buildvfs_fgetc(buildvfs_FILE fp)
{
  unsigned char c;
  return buildvfs_fread(&c, 1, 1, fp) != 1 ? buildvfs_EOF : c;
}
static inline int buildvfs_fputc(char c, buildvfs_FILE fp)
{
    return PHYSFS_writeBytes(fp, &c, 1) != 1 ? buildvfs_EOF : c;
}
#define buildvfs_fclose(fp) PHYSFS_close(fp)
#define buildvfs_feof(fp) PHYSFS_eof(fp)
#define buildvfs_ftell(fp) PHYSFS_tell(fp)
#define buildvfs_fseek_abs(fp, o) PHYSFS_seek((fp), (o))
#define buildvfs_fseek_rel(fp, o) PHYSFS_seek((fp), PHYSFS_tell(fp) + (o))
#define buildvfs_rewind(fp) PHYSFS_seek((fp), 0)

#define buildvfs_flength(fp) PHYSFS_fileLength(fp)

#define buildvfs_chdir(dir) (-1)
#define buildvfs_mkdir(dir, x) (!PHYSFS_mkdir(dir))
static inline char *buildvfs_getcwd(char *buf, size_t size)
{
    if (buf == nullptr || size == 0)
        return nullptr;

    buf[0] = '\0';
    return buf;
}

using buildvfs_fd = PHYSFS_File *;
#define buildvfs_fd_invalid (nullptr)
#define buildvfs_read(fd, p, s) PHYSFS_readBytes((fd), (p), (s))
#define buildvfs_write(fd, p, s) PHYSFS_writeBytes((fd), (p), (s))
#define buildvfs_open_read(fn) PHYSFS_openRead(fn)
#define buildvfs_open_write(fn) PHYSFS_openWrite(fn)
#define buildvfs_open_append(fn) PHYSFS_openAppend(fn)
#define buildvfs_close(fd) PHYSFS_close(fd)
#define buildvfs_tell(fd) PHYSFS_tell(fd)
static inline int64_t buildvfs_lseek_abs(buildvfs_fd fd, int64_t o)
{
    PHYSFS_seek(fd, o);
    return PHYSFS_tell(fd);
}
static inline int64_t buildvfs_lseek_rel(buildvfs_fd fd, int64_t o)
{
    PHYSFS_seek(fd, PHYSFS_tell(fd) + o);
    return PHYSFS_tell(fd);
}

#define buildvfs_length(fd) PHYSFS_fileLength(fd)
#define buildvfs_exists(fn) PHYSFS_exists(fn)
#define buildvfs_isdir(path) PHYSFS_isDirectory(path)
#define buildvfs_unlink(path) PHYSFS_delete(path)

#else

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#ifdef _WIN32
# include <io.h>
#endif

using buildvfs_FILE = FILE *;
#define buildvfs_EOF EOF
#define buildvfs_fread(p, s, n, fp) fread((p), (s), (n), (fp))
#define buildvfs_fwrite(p, s, n, fp) fwrite((p), (s), (n), (fp))
#define buildvfs_fopen_read(fn) fopen((fn), "rb")
#define buildvfs_fopen_write(fn) fopen((fn), "wb")
#define buildvfs_fopen_write_text(fn) fopen((fn), "w")
#define buildvfs_fopen_append(fn) fopen((fn), "ab")
#define buildvfs_fgetc(fp) fgetc(fp)
#define buildvfs_fputc(c, fp) fputc((c), (fp))
#define buildvfs_fgets(str, size, fp) fgets((str), (size), (fp))
#define buildvfs_fclose(fp) fclose(fp)
#define buildvfs_feof(fp) feof(fp)
#define buildvfs_ftell(fp) ftell(fp)
#define buildvfs_fseek_abs(fp, o) fseek((fp), (o), SEEK_SET)
#define buildvfs_fseek_rel(fp, o) fseek((fp), (o), SEEK_CUR)
#define buildvfs_rewind(fp) rewind(fp)

static inline int64_t buildvfs_length(int fd)
{
#ifdef _WIN32
    return filelength(fd);
#else
    struct stat st;
    return fstat(fd, &st) < 0 ? -1 : st.st_size;
#endif
}

#define buildvfs_chdir(dir) chdir(dir)
#define buildvfs_mkdir(dir, x) Bmkdir(dir, x)
#define buildvfs_getcwd(buf, size) getcwd((buf), (size))

using buildvfs_fd = int;
#define buildvfs_fd_invalid (-1)
#define buildvfs_read(fd, p, s) read((fd), (p), (s))
#define buildvfs_write(fd, p, s) write((fd), (p), (s))
#define buildvfs_open_read(fn) open((fn), O_RDONLY)
#define buildvfs_open_write(fn) open((fn), O_BINARY|O_TRUNC|O_CREAT|O_WRONLY, S_IREAD|S_IWRITE)
// #define buildvfs_open_append(fn) todo(fn)
#define buildvfs_close(fd) close(fd)
#define buildvfs_tell(fd) lseek((fd), 0, SEEK_CUR)
#define buildvfs_lseek_abs(fd, o) lseek((fd), (o), SEEK_SET)
#define buildvfs_lseek_rel(fd, o) lseek((fd), (o), SEEK_CUR)

static inline int64_t buildvfs_flength(FILE * f)
{
#ifdef _WIN32
    return filelength(_fileno(f));
#else
    return buildvfs_length(fileno(f));
#endif
}
#define buildvfs_exists(fn) (access((fn), F_OK) == 0)
static inline int buildvfs_isdir(char const *path)
{
    struct Bstat st;
    return (Bstat(path, &st) ? 0 : (st.st_mode & S_IFDIR) == S_IFDIR);
}
#define buildvfs_unlink(path) unlink(path)

#endif

#define MAYBE_FCLOSE_AND_NULL(fileptr) do { \
    if (fileptr) { buildvfs_fclose(fileptr); fileptr = buildvfs_FILE{}; } \
} while (0)

static inline void buildvfs_fputstrptr(buildvfs_FILE fp, char const * str)
{
    buildvfs_fwrite(str, 1, strlen(str), fp);
}

static inline void buildvfs_fputs(char const * str, buildvfs_FILE fp)
{
    buildvfs_fwrite(str, 1, strlen(str), fp);
}

template <size_t N>
static inline void buildvfs_fputstr(buildvfs_FILE fp, char const (&str)[N])
{
    buildvfs_fwrite(&str, 1, N-1, fp);
}

#endif // vfs_h_
