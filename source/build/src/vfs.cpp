
#include "vfs.h"
#include "cache1d.h"

#ifdef USE_PHYSFS

int32_t numgroupfiles;

void uninitgroupfile(void)
{
    PHYSFS_deinit();
}

#include <sys/stat.h>

int32_t klseek(buildvfs_kfd handle, int32_t offset, int32_t whence)
{
    // TODO: replace klseek calls with _{abs,cur,end} versions

    if (whence == SEEK_CUR)
        offset += PHYSFS_tell(handle);
    else if (whence == SEEK_END)
        offset += PHYSFS_fileLength(handle);

    PHYSFS_seek(handle, offset);
    return PHYSFS_tell(handle);
}

#endif
