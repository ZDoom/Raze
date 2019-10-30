
#pragma once

#ifndef vfs_h_
#define vfs_h_

#include "compat.h"


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

#define buildvfs_getcwd(buf, size) getcwd((buf), (size))

using buildvfs_fd = int;
#define buildvfs_fd_invalid (-1)
#define buildvfs_read(fd, p, s) read((fd), (p), (s))
#define buildvfs_write(fd, p, s) write((fd), (p), (s))
#define buildvfs_open_read(fn) open((fn), O_RDONLY)
#define buildvfs_open_write(fn) open((fn), O_BINARY|O_TRUNC|O_CREAT|O_WRONLY, S_IREAD|S_IWRITE)
// #define buildvfs_open_append(fn) todo(fn)
#define buildvfs_close(fd) close(fd)

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
