-- Virtual file system facilities for Lunatic

local ffi = require("ffi")
local C = ffi.C

local decl = require("lprivate").decl

local assert = assert
local error = error
local type = type

----------

decl[[
int32_t pathsearchmode;

typedef struct _CACHE1D_FIND_REC {
	char *name;
	int32_t type, source;
	struct _CACHE1D_FIND_REC *next, *prev, *usera, *userb;
} CACHE1D_FIND_REC;

void klistfree(CACHE1D_FIND_REC *rec);
CACHE1D_FIND_REC *klistpath(const char *path, const char *mask, int32_t type);
]]


-- The API table
local fs = {}

local CACHE1D = ffi.new[[
struct {
	static const int FIND_FILE = 1;
	static const int FIND_DIR = 2;
	static const int FIND_DRIVE = 4;
	static const int FIND_NOCURDIR = 8;

	static const int OPT_NOSTACK = 0x100;

	// the lower the number, the higher the priority
	static const int SOURCE_DRIVE = 0;
	static const int SOURCE_CURDIR = 1;
	static const int SOURCE_PATH = 2;  // + path stack depth
	static const int SOURCE_ZIP = 0x7ffffffe;
	static const int SOURCE_GRP = 0x7fffffff;
}
]]

-- files = fs.listpath(path, mask)
-- TODO: filter by 'source' (path, zip and/or grp)
-- TODO: directories too, so that one can list them recursively, for example
function fs.listpath(path, mask)
    if (type(path)~="string") then
        -- TODO: maybe also allow nil
        error("Invalid argument #1: must be a string", 2)
    end

    if (type(mask) ~= "string") then
        error("Invalid argument #2: must be a string", 2)
    end

    -- Normalization, for portability's sake

    if (path:find("\\")) then
        error("Invalid argument #1: must not contain backslashes", 2)
    end

    if (mask:find("[\\/]")) then
        error("Invalid argument #2: must not contain back or forward slashes", 2)
    end

    local opsm = C.pathsearchmode
    C.pathsearchmode = 0
    local rootrec = C.klistpath(path, mask, CACHE1D.FIND_FILE + CACHE1D.FIND_NOCURDIR)
    C.pathsearchmode = opsm

    if (rootrec == nil) then
        -- XXX: may have failed hard or just no matching files
        return {}
    end

    local files = {}

    local rec = rootrec
    while (rec ~= nil) do
        assert(rec.name ~= nil)
        files[#files+1] = ffi.string(rec.name)
        rec = rec.next
    end

    C.klistfree(rootrec)

    return files
end

return fs
