-- LunaCON CON to Lunatic translator
-- requires LPeg, http://www.inf.puc-rio.br/~roberto/lpeg/lpeg.html

local require = require
local lpeg = require("lpeg")

local bit
local math = require("math")
local string = require("string")
local table = require("table")


local arg = arg

local assert = assert
local pairs = pairs
local pcall = pcall
local print = print
local tonumber = tonumber
local tostring = tostring
local type = type
local unpack = unpack

-- non-nil if running from EDuke32
-- (read_into_string~=nil  iff  string.dump==nil)
local read_into_string = read_into_string
local ffi, ffiC

if (string.dump) then
    bit = require("bit")
    -- For Rio Lua:
--    bit = { bor=function() return 0 end }
    require("strict")
else
    bit = require("bit")
    ffi = require("ffi")
    ffiC = ffi.C
end


module("lunacon")


-- I think that the "too many pending calls/choices" is unavoidable in general.
-- This limit is of course still arbitrary, but writing long if/else cascades
-- in CON isn't pretty either (though sometimes necessary because nested switches
-- don't work?)
-- See also:  http://lua-users.org/lists/lua-l/2010-03/msg00086.html
lpeg.setmaxstack(1024);


local Pat, Set, Range, Var = lpeg.P, lpeg.S, lpeg.R, lpeg.V
local POS = lpeg.Cp

---- All keywords pattern -- needed for CON syntax
local conl = require("con_lang")


local function match_until(matchsp, untilsp)  -- (!untilsp matchsp)* in PEG
    -- sp: string or pattern
    return (matchsp - Pat(untilsp))^0
end

local format = string.format

local function printf(fmt, ...)
    print(format(fmt, ...))
end


---=== semantic action functions ===---

local inf = 1/0
local NaN = 0/0

-- Last keyword position, for error diagnosis.
local g_lastkwpos = nil
local g_lastkw = nil
local g_badids = {}  -- maps bad id strings to 'true'

local g_recurslevel = -1  -- 0: base CON file, >0 included
local g_filename = "???"
local g_directory = ""  -- with trailing slash if not empty
local g_maxerrors = 20
local g_numerrors = 0

-- Warning options. Key names are the same as cmdline options, e.g.
-- -Wno-bad-identifier for disabling the "bad identifier" warning.
local g_warn = { ["not-redefined"]=true, ["bad-identifier"]=true,
                 ["number-conversion"]=true, }

-- How many 'if' statements are following immediately each other,
-- needed to cope with CONs dangling-else resolution
local g_iflevel = 0
local g_ifelselevel = 0

---=== Code generation ===---
local g_have_file = {}  -- [filename]=true
local g_curcode = nil  -- a table of string pieces or other "gencode" tables

local g_actor_code = {}  -- [actornum]=gencode_table
local g_event_code = {}  -- [eventnum]=gencode_table
local g_loadactor_code = {}  -- [actornum]=gencode_table


local function getlinecol(pos) end -- fwd-decl

local function new_initial_codetab()
    return {
        "local _con, _bit, _math = require'con', require'bit', require'math';",
        "local sector, sprite, actor, player = sector, sprite, actor, player;",
        "local gameactor=gameactor;"
           }
end

local function reset_codegen()
    g_have_file = {}
    g_curcode = new_initial_codetab()
    g_actor_code, g_event_code, g_loadactor_code = {}, {}, {}

    g_recurslevel = -1
    g_numerrors = 0
end

local function addcode(x)
    assert(type(x)=="string" or type(x)=="table")
    g_curcode[#g_curcode+1] = x
end

local function addcodef(fmt, ...)
    addcode(format(fmt, ...))
end

local function add_code_and_end(codetab, endstr)
    assert(type(codetab)=="table")
    addcode(codetab)
    addcode(endstr)
end

local function on_actor_end(usertype, tsamm, codetab)
    local tilenum = tsamm[1]

    local str = ""
    for i=2,math.min(#tsamm,4) do
        if ((i==3 or i==4) and tsamm[i]=="0") then
            -- HACK, gameactor() currently doesn't support literals
            tsamm[i] = "'NO'"
        end
        str = str .. tostring(tsamm[i])..","
    end
    if (#tsamm==5) then
        local flags = bit.bor(unpack(tsamm, 5))
        str = str .. flags..","
    end

    -- TODO: usertype (is non-nil only for 'useractor')
    addcodef("gameactor(%d,%sfunction(_aci, _pli, _dist)", tilenum, str)
    add_code_and_end(codetab, "end)")

    g_actor_code[tilenum] = codetab
end

local function on_state_end(statename, codetab)
    -- TODO: mangle names
    addcodef("local function %s(_aci, _pli, _dist)", statename)
    add_code_and_end(codetab, "end")
end

local function on_event_end(eventidx, codetab)
    addcodef("gameevent(%d, function (_aci, _pli, _dist)", eventidx)
    add_code_and_end(codetab, "end)")

    g_event_code[eventidx] = codetab
end

----------

local function linecolstr(pos)
    local line, col = getlinecol(pos)
    return format("%d:%d", line, col)
end

local function increment_numerrors()
    g_numerrors = g_numerrors+1
    if (g_numerrors == g_maxerrors) then
        g_numerrors = inf
        printf("Too many errors (%d), aborting...", g_maxerrors)
    end
end

local function perrprintf(pos, fmt, ...)
    printf("%s %s: error: "..fmt, g_filename, linecolstr(pos), ...)
    increment_numerrors()
end

local function errprintf(fmt, ...)
    if (g_lastkwpos) then
        perrprintf(g_lastkwpos, fmt, ...)
    else
        printf("%s ???: error: "..fmt, g_filename, ...)
    end
    increment_numerrors()
end

local function pwarnprintf(pos, fmt, ...)
    printf("%s %s: warning: "..fmt, g_filename, linecolstr(pos), ...)
end

local function warnprintf(fmt, ...)
    if (g_lastkwpos) then
        pwarnprintf(g_lastkwpos, fmt, ...)
    else
        printf("%s ???: warning: "..fmt, g_filename, ...)
    end
end

local function parse_number(pos, numstr)
    local num = tonumber(numstr)

    if (num==nil or num < -0x80000000 or num > 0xffffffff) then
        perrprintf(pos, "number %s out of the range of a 32-bit integer", numstr)
        num = NaN
    elseif (num >= 0x80000000 and numstr:sub(1,2):lower()~="0x") then
        if (g_warn["number-conversion"]) then
            pwarnprintf(pos, "number %s converted to a negative one", numstr)
        end
        num = num-(0xffffffff+1)
    end

    return num
end


-- Mapping of various "define" types to the respective number of members and
-- vice versa
local LABEL = { MOVE=2, AI=3, ACTION=5, [2]="move", [3]="ai", [5]="action",
                NUMBER=1, [1]="number" }

-- Function names in the 'con' module
local LABEL_FUNCNAME = { [2]="move", [3]="ai", [5]="action" }

local g_labeldef = {}  -- Lua numbers for numbers, strings for composites
local g_labeltype = {}

local function reset_labels()
    g_badids = {}

    -- NO is also a valid `move', `ai' or `action', but they are handled
    -- separately in lookup_composite().
    g_labeldef = { NO=0 }
    g_labeltype = { NO=LABEL.NUMBER }

    -- Initialize default defines.
    for i=1,#conl.labels do
        for label, val in pairs(conl.labels[i]) do
            g_labeldef[label] = val
            g_labeltype[label] = LABEL.NUMBER
        end
    end
end

local function lookup_defined_label(pos, maybe_minus_str, identifier)
    local num = g_labeldef[identifier]

    if (num == nil) then
        perrprintf(pos, "label \"%s\" is not defined", identifier)
        return -inf  -- return a number for type cleanness
    end

    if (g_labeltype[identifier] ~= LABEL.NUMBER) then
        perrprintf(pos, "label \"%s\" is not a `define'd number", identifier)
        return -inf
    end

    assert(type(num)=="number")

    return (maybe_minus_str=="" and 1 or -1) * num
end

local function do_define_label(identifier, num)
    local oldtype = g_labeltype[identifier]
    local oldval = g_labeldef[identifier]

    if (oldval) then
        if (oldtype ~= LABEL.NUMBER) then
            errprintf("refusing to overwrite `%s' label \"%s\" with a `define'd number",
                      LABEL[oldtype], identifier)
        else
            -- conl.labels[...]: don't warn for wrong PROJ_ redefinitions
            if (g_warn["not-redefined"]) then
                if (oldval ~= num and conl.PROJ[identifier]==nil) then
                    warnprintf("label \"%s\" not redefined with new value %d (old: %d)",
                               identifier, num, oldval)
                end
            end
        end
    else
        -- New definition of a label
        g_labeldef[identifier] = num
        g_labeltype[identifier] = LABEL.NUMBER
    end
end

local function check_composite_literal(labeltype, pos, num)
    if (num==0 or num==1) then
        return (num==0) and "0" or "1"
    else
        perrprintf(pos, "literal `%s' number must be either 0 or 1", LABEL[labeltype])
        return "_INVALIT"
    end
end

local function lookup_composite(labeltype, pos, identifier)
    if (identifier=="NO") then
        -- NO is a special case and is valid for move, action and ai,
        -- being the same as passing a literal 0.
        return "0"
    end

    local val = g_labeldef[identifier]

    if (val == nil) then
        perrprintf(pos, "label \"%s\" is not defined", identifier)
        return "_NOTDEF"
    elseif (g_labeltype[identifier] ~= labeltype) then
        if (identifier=="randomangle" and labeltype==LABEL.MOVE) then
            pwarnprintf(pos, "label \"randomangle\" is not a `move' value, assuming 0")
            return "0"
        else
            perrprintf(pos, "label \"%s\" is not a%s `%s' value", identifier,
                       labeltype==LABEL.MOVE and "" or "n", LABEL[labeltype])
            return "_WRONGTYPE"
        end
    end

    -- Generate a quoted identifier name.
    val = format("%q", identifier)

    return val
end

local function do_define_composite(labeltype, identifier, ...)
    local oldtype = g_labeltype[identifier]
    local oldval = g_labeldef[identifier]

    if (oldval) then
        if (oldtype ~= labeltype) then
            errprintf("refusing to overwrite `%s' label \"%s\" with a `%s' value",
                      LABEL[oldtype], identifier, LABEL[labeltype])
        else
            warnprintf("duplicate `%s' definition of \"%s\" ignored",
                       LABEL[labeltype], identifier)
        end
        return
    end

    -- Fill up omitted arguments with zeros.
    local isai = (labeltype == LABEL.AI)
    local args = {...}
    for i=#args+1,labeltype do
        -- passing nil to con.ai will make the action/move the null one
        args[i] = (isai and i<=2) and "nil" or 0
    end

    if (isai) then
        assert(type(args[1])=="string")
        assert(type(args[2])=="string")

        -- OR together the flags
        for i=#args,LABEL.AI+1, -1 do
            -- TODO: check?
            args[LABEL.AI] = bit.bor(args[LABEL.AI], args[i])
            args[i] = nil
        end
    end

    -- Make a string out of that.
    for i=1+(isai and 2 or 0),#args do
        args[i] = format("%d", args[i])
    end

    addcodef("_con.%s(%q,%s)", LABEL_FUNCNAME[labeltype], identifier, table.concat(args, ","))

    g_labeldef[identifier] = ""
    g_labeltype[identifier] = labeltype
end


local function parse(contents) end -- fwd-decl

local function do_include_file(dirname, filename)
    assert(type(filename)=="string")

    if (g_have_file[filename] ~= nil) then
        printf("[%d] Fatal error: infinite loop including \"%s\"", filename)
        g_numerrors = inf
        return
    end

    local contents

    if (read_into_string) then
        -- running from EDuke32
        contents = read_into_string(filename)
    else
        -- running stand-alone
        local io = require("io")

        local fd, msg = io.open(dirname..filename)
        if (fd == nil) then
            -- strip up to and including first slash:
            filename = string.gsub(filename, "^.-/", "")
            fd, msg = io.open(dirname..filename)

            if (fd == nil) then
                printf("[%d] Fatal error: couldn't open %s", g_recurslevel, msg)
                g_numerrors = inf
                return
            end
        end

        contents = fd:read("*all")
        fd:close()
    end

    if (contents == nil) then
        -- maybe that file name turned out to be a directory or other
        -- special file accidentally
        printf("[%d] Fatal error: couldn't read from \"%s\"",
               g_recurslevel, dirname..filename)
        g_numerrors = inf
        return
    end

    printf("%s[%d] Translating file \"%s\"", (g_recurslevel==-1 and "\n---- ") or "",
           g_recurslevel+1, dirname..filename);

    local oldfilename = g_filename
    g_filename = filename
    parse(contents)
    g_filename = oldfilename
end

function cmd_include(filename)
    do_include_file(g_directory, filename)
end

--- Per-module game data
local g_data = {}
local EPMUL = conl.MAXLEVELS

local function reset_gamedata()
    g_data = {}

    -- [EPMUL*ep + lev] = { ptime=<num>, dtime=<num>, fn=<str>, name=<str> }
    g_data.level = {}
    -- [ep] = <str>
    g_data.volname = {}
    -- [skillnum] = <str>
    g_data.skillname = {}
    -- [quotenum] = <str>
    g_data.quote = {}
    -- table of length 26 or 30 containg numbers
    g_data.startup = {}
    -- [soundnum] = { fn=<str>, params=<table of length 5> }
    g_data.sound = {}
    -- [volnum] = <table of length numlevels (<= MAXLEVELS) of <str>>
    g_data.music = {}
end

local function cmd_definelevelname(vol, lev, fn, ptstr, dtstr, levname)
    if (vol < 0 or vol >= conl.MAXVOLUMES) then
        errprintf("volume number exceeds maximum volume count.")
        return
    end

    if (lev < 0 or lev >= conl.MAXLEVELS) then
        errprintf("level number exceeds maximum number of levels per episode.")
        return
    end

    -- TODO: Bcorrectfilename(fn)

    local function secs(tstr)
        local m, s = string.match(tstr, ".+:.+")
        m, s = tonumber(m), tonumber(s)
        return (m and s) and m*60+s or 0
    end

    local map = {
        ptime=secs(ptstr), dtime=secs(dtstr), fn="/"..fn, name=levname
    }

    if (ffi) then
        ffiC.C_DefineLevelName(vol, lev, map.fn, map.ptime, map.dtime, map.name)
    end

    g_data.level[EPMUL*vol+lev] = map
end

local function cmd_defineskillname(skillnum, name)
    if (skillnum < 0 or skillnum >= conl.MAXSKILLS) then
        errprintf("volume number is negative or exceeds maximum skill count.")
        return
    end

    g_data.skillname[skillnum] = name:upper()
end

local function cmd_definevolumename(vol, name)
    if (vol < 0 or vol >= conl.MAXVOLUMES) then
        errprintf("volume number is negative or exceeds maximum volume count.")
        return
    end

    name = name:upper()
    if (ffi) then
        ffiC.C_DefineVolumeName(vol, name)
        if (#name > 32) then
            warnprintf("volume %d name truncated to 32 characters.", vol)
        end
    end

    g_data.volname[vol] = name
end

-- strip whitespace from front and back
local function stripws(str)
    return str:match("^%s*(.*)%s*$")
end

local function cmd_definequote(qnum, quotestr)
--[[
    -- have the INT_MAX limit simply for some sanity
    if (not (qnum >= 0 and <= 0x7fffffff)) then
        errprintf("quote number is negative or exceeds limit of INT32_MAX.")
        return
    end
--]]
    if (not (qnum >= 0 and qnum < conl.MAXQUOTES)) then
        errprintf("quote number is negative or exceeds limit of %d.", conl.MAXQUOTES-1)
        return
    end

    quotestr = stripws(quotestr)

    if (ffi) then
        if (#quotestr >= conl.MAXQUOTELEN) then
            warnprintf("quote %d truncated to %d characters.", conl.MAXQUOTELEN-1)
        end

        ffiC.C_DefineQuote(qnum, quotestr)
    end

    g_data.quote[qnum] = quotestr
end

local function cmd_gamestartup(...)
    local args = {...}

    if (#args ~= 26 and #args ~= 30) then
        errprintf("must pass either 26 (1.3D) or 30 (1.5) values")
        return
    end

    if (ffi) then
        -- running from EDuke32
        if (#args == 30) then
            ffiC.g_scriptVersion = 14
        end
        local params = ffi.new("int32_t [30]", args)
        ffiC.G_DoGameStartup(params)
    end

    g_data.startup = args  -- TODO: sanity-check them
end

local function cmd_definesound(sndnum, fn, ...)
    if (not (sndnum >= 0 and sndnum < conl.MAXSOUNDS)) then
        errprintf("sound number is negative or exceeds sound limit of %d", conl.MAXSOUNDS-1)
        return
    end

    local params = {...}  -- TODO: sanity-check them
    if (ffi) then
        local cparams = ffi.new("int32_t [5]", params)
        assert(type(fn)=="string")
        ffiC.C_DefineSound(sndnum, fn, cparams)
    end

    g_data.sound[sndnum] = { fn=fn, params=params }
end

local function cmd_music(volnum, ...)
    if (not (volnum >= 0 and volnum < conl.MAXVOLUMES+1)) then
        -- NOTE: MAXVOLUMES is OK, since it's MapInfo[(MAXVOLUMES+1)*MAXLEVELS]
        errprintf("volume number is negative or exceeds MAXVOLUMES=%d", conl.MAXVOLUMES)
        return
    end

    local filenames = {...}

    if (#filenames > conl.MAXLEVELS) then
        warnprintf("ignoring extraneous %d music file names", #filenames-conl.MAXLEVELS)
        for i=conl.MAXLEVELS+1,#filenames do
            filenames[i] = nil
        end
    end

    if (ffi) then
        for i=1,#filenames do
            assert(type(filenames[i])=="string")
            ffiC.C_DefineMusic(volnum, i-1, "/"..filenames[i])
        end
    end

    g_data.music[volnum] = filenames
end


----==== patterns ====----

---- basic ones
-- Windows, *nix and Mac newlines all exist in the wild!
local newline = "\r"*Pat("\n")^-1 + "\n"
local EOF = Pat(-1)
local anychar = Pat(1)
-- comments
local comment = "/*" * match_until(anychar, "*/") * "*/"
local linecomment = "//" * match_until(anychar, newline)
local whitespace = Var("whitespace")
local sp0 = whitespace^0
-- This "WS+" pattern matches EOF too, so that a forgotten newline at EOF is
-- properly handled
local sp1 = whitespace^1 + EOF
local alpha = Range("AZ", "az")  -- locale?
local alphanum = alpha + Range("09")
--local alnumtok = alphanum + Set("{}/\\*-_.")  -- see isaltok() in gamedef.c

--- basic lexical elements ("tokens")
local t_maybe_minus = (Pat("-") * sp0)^-1;
local t_number = POS() * lpeg.C(
    t_maybe_minus * ((Pat("0x") + "0X")*Range("09", "af", "AF")^1 + Range("09")^1)
                       ) / parse_number
-- Valid identifier names are disjunct from keywords!
-- XXX: CON is more permissive with identifier name characters:
local t_identifier = Var("t_identifier")
-- This one matches keywords, too:
local t_identifier_all = Var("t_identifier_all")
local t_define = Var("t_define")
local t_move = Var("t_move")
local t_ai = Var("t_ai")
local t_action = Var("t_action")
-- NOTE: no chance to whitespace and double quotes in filenames:
local t_filename = lpeg.C((anychar-Set(" \t\r\n\""))^1)
local t_newline_term_str = match_until(anychar, newline)

-- new-style inline arrays and structures:
local t_arrayexp = Var("t_arrayexp")

-- defines and constants can take the place of vars that are only read:
-- NOTE: when one of t_identifier+t_define matches, we don't actually know
--  whether it's the right one yet, since their syntax overlaps.
local t_rvar = t_arrayexp + t_identifier + t_define
-- not so with written-to vars:
local t_wvar = t_arrayexp + t_identifier


---- helper patterns / pattern constructing functions
local maybe_quoted_filename = ('"' * t_filename * '"' + t_filename)
-- empty string is handled too; we must not eat the newline then!
local newline_term_string = (#newline + EOF)*lpeg.Cc("")
    + (whitespace-newline)^1 * lpeg.C(t_newline_term_str)


-- (sp1 * t_define) repeated exactly n times
local function n_defines(n)  -- works well only for small n
    local pat = Pat(true)
    for i=1,n do
        pat = sp1 * t_define * pat
    end
    return pat
end


local D, R, W, I, AC, MV, AI = -1, -2, -3, -4, -5, -6, -7
local TOKEN_PATTERN = { [D]=t_define, [R]=t_rvar, [W]=t_wvar, [I]=t_identifier,
                        [AC]=t_action, [MV]=t_move, [AI]=t_ai }

-- Generic command pattern, types given by varargs.
-- The command name to be matched is attached later.
-- Example:
--  "command" writtenvar readvar def def:  gencmd(W,R,D,D)
--    -->  sp1 * t_wvar * sp1 * t_rvar * sp1 * t_define * sp1 * t_define
--  "command_with_no_args":  gencmd()
--    --> Pat(true)
local function cmd(...)
    local pat = Pat(true)
    local vartypes = {...}

    for i=1,#vartypes do
        pat = pat * sp1 * assert(TOKEN_PATTERN[vartypes[i]])
    end

    return pat
end


local t_time = lpeg.C(alphanum*alphanum^-1*":"*alphanum*alphanum^-1)  -- for definelevelname

-- The command names will be attached to the front of the patterns later!

--== Top level CON commands ==--
-- XXX: many of these are also allowed inside actors/states/events in CON.
local Co = {
    --- 1. Preprocessor
    include = sp1 * maybe_quoted_filename / cmd_include,
    includedefault = cmd(),
    define = cmd(I,D) / do_define_label,

    --- 2. Defines and Meta-Settings
    dynamicremap = cmd(),
    setcfgname = sp1 * t_filename,
    setdefname = sp1 * t_filename,
    setgamename = newline_term_string,

    precache = cmd(D,D,D),
    scriptsize = cmd(D)
        / "",  -- no-op
    cheatkeys = cmd(D,D),

    definecheat = newline_term_string,  -- XXX: actually tricker syntax (TS)
    definegamefuncname = newline_term_string,  -- XXX: TS?
    definegametype = n_defines(2) * newline_term_string,
    definelevelname = n_defines(2) * sp1 * t_filename * sp1 * t_time * sp1 * t_time *
        newline_term_string / cmd_definelevelname,
    defineskillname = sp1 * t_define * newline_term_string / cmd_defineskillname,
    definevolumename = sp1 * t_define * newline_term_string / cmd_definevolumename,

    definequote = sp1 * t_define * newline_term_string / cmd_definequote,
    defineprojectile = cmd(D,D,D),
    definesound = sp1 * t_define * sp1 * maybe_quoted_filename * n_defines(5) / cmd_definesound,

    -- NOTE: gamevar.ogg and the like is OK, too
    music = sp1 * t_define * match_until(sp1 * t_filename, sp1 * conl.keyword * sp1) / cmd_music,

    --- 3. Game Settings
    -- gamestartup has 26/30 fixed defines, depending on 1.3D/1.5 version:
    gamestartup = (sp1 * t_define)^26 / cmd_gamestartup,
    spritenopal = cmd(D),
    spritenoshade = cmd(D),
    spritenvg = cmd(D),
    spriteshadow = cmd(D),

    spriteflags = cmd(D,D),  -- also see inner

    --- 4. Game Variables / Arrays
    gamevar = cmd(I,D,D),
    gamearray = cmd(I,D),

    --- 5. Top level commands that are also run-time commands
    move = sp1 * t_identifier * (sp1 * t_define)^-2 /  -- hvel, vvel
        function(...) do_define_composite(LABEL.MOVE, ...) end,

    -- startframe, numframes, viewtype, incval, delay:
    action = sp1 * t_identifier * (sp1 * t_define)^-5 /
        function(...) do_define_composite(LABEL.ACTION, ...) end,

    -- action, move, flags...:
    ai = sp1 * t_identifier * (sp1 * t_action *
                               (sp1 * t_move * (sp1 * t_define)^0)^-1
                              )^-1 /
        function(...) do_define_composite(LABEL.AI, ...) end,

    --- 6. Deprecated TLCs
    betaname = newline_term_string,
    enhanced = cmd(D),
}


--== Run time CON commands ==--
--- 1. Gamevar Operators
local varop = cmd(W,D)
local varvarop = cmd(W,R)

-- Allow nesting... stuff like
--   ifvarl actorvar[sprite[THISACTOR].owner].burning 0
-- is kinda breaking the classic "no array nesting" rules
-- (if there ever were any) but making our life harder else.
local arraypat = sp0 * "[" * sp0 * t_rvar * sp0 * "]"

-- Have to bite the bullet here and list actor/player members with second parameters,
-- even though it's ugly to make it part of the syntax.  Also, stuff like
--   actor[xxx].loogiex parm2 x
-- will be wrongly accepted at the parsing stage because we don't discriminate between
-- actor and player (but it will be rejected later).
local parm2memberpat = (Pat("htg_t") + "loogiex" + "loogiey" + "ammo_amount" +
                        "weaprecs" + "gotweapon" + "pals" + "max_ammo_amount") * sp1 * t_rvar
-- The member name must match keywords, too (_all), because e.g. cstat is a member
-- of sprite[].
local memberpat = sp0 * "." * sp0 * (parm2memberpat + t_identifier_all)

local getstructcmd =  -- get<structname>[<idx>].<member> (<parm2>)? <<var>>
    -- existence of a second parameter is determined later
-- This is wrong,  (sp1 id)? will match (sp1 wvar) if there's no 2nd param:
--    arraypat * memberpat * (sp1 * t_identifier)^-1 * sp1 * t_wvar
    arraypat * memberpat * sp1 * (t_rvar * sp1 * t_wvar + t_wvar)

local setstructcmd =  -- set<structname>[<idx>].<member> (<parm2>)? <var>
    -- existence of a second parameter is determined later
    arraypat * memberpat * sp1 * (t_rvar * sp1 * t_rvar + t_rvar)

local getperxvarcmd =  -- get<actor/player>var[<idx>].<member> <<var>>
    arraypat * memberpat * sp1 * t_wvar

local setperxvarcmd = -- set<actor/player>var[<idx>].<member> <var>
    arraypat * memberpat * sp1 * t_rvar


local function ACS(s) return "actor[_aci]"..s end
local function SPS(s) return "sprite[_aci]"..s end
local function PLS(s) return "player[_pli]"..s end

local function handle_palfrom(...)
    local v = {...}
    return format(PLS":_palfrom(%d,%d,%d,%d)",
                  v[1] or 0, v[2] or 0, v[3] or 0, v[4] or 0)
end

local function handle_move(mv, ...)
    local flags = {...}
    return format(ACS":set_move(%s,%d)", mv, (flags[1] and bit.bor(...)) or 0)
end

local function handle_debug(val)
    return format("print('%s:%d: debug %d')", g_filename, getlinecol(g_lastkwpos), val)
end

-- NOTE about prefixes: most is handled by all_alt_pattern(), however commands
-- that have no arguments and that are prefixes of other commands MUST be
-- suffixed with a "* #sp1" pattern.

local Ci = {
    -- these can appear anywhere in the script
    ["break"] = cmd()
        / "do return end",
    ["return"] = cmd()  -- NLCF
        / "_con.longjmp()",  -- TODO: test with code from Wiki "return" entry

    state = cmd(I)
        / "%1(_aci, _pli, _dist)",  -- TODO: mangle names

    --- 1. get*, set*
    getactor = getstructcmd,
    getinput = getstructcmd,
    getplayer = getstructcmd,
    getprojectile = getstructcmd,
    getsector = getstructcmd,
    getthisprojectile = getstructcmd,
    gettspr = getstructcmd,
    -- NOTE: {get,set}userdef is the only struct that can be accessed without
    -- an "array part", e.g.  H266MOD has "setuserdef .weaponswitch 0" (space
    -- between keyword and "." is mandatory)
    getuserdef = (arraypat + sp1) * memberpat * sp1 * (t_rvar * sp1 * t_wvar + t_wvar),
--    getuserdef = getstructcmd,
    getwall = getstructcmd,

    getactorvar = getperxvarcmd,
    getplayervar = getperxvarcmd,

    setactor = setstructcmd,
    setinput = setstructcmd,
    setplayer = setstructcmd,
    setprojectile = setstructcmd,
    setsector = setstructcmd,
    setthisprojectile = setstructcmd,
    settspr = setstructcmd,
    setuserdef = (arraypat + sp1) * memberpat * sp1 * (t_rvar * sp1 * t_wvar + t_rvar),
--    setuserdef = setstructcmd,
    setwall = setstructcmd,

    setactorvar = setperxvarcmd,
    setplayervar = setperxvarcmd,

    setsprite = cmd(R,R,R,R),

    setvarvar = varvarop,
    addvarvar = varvarop,
    subvarvar = varvarop,
    mulvarvar = varvarop,
    divvarvar = varvarop,
    modvarvar = varvarop,
    andvarvar = varvarop,
    orvarvar = varvarop,
    xorvarvar = varvarop,
    randvarvar = varvarop,

    setvar = varop,
    addvar = varop,
    subvar = varop,
    mulvar = varop,
    divvar = varop,
    modvar = varop,
    andvar = varop,
    orvar = varop,
    xorvar = varop,
    randvar = varop,
    shiftvarl = varop,
    shiftvarr = varop,

    --- 2. Math operations
    sqrt = cmd(R,W),
    calchypotenuse = cmd(W,R,R),
    sin = cmd(W,R),
    cos = cmd(W,R),
    mulscale = cmd(W,R,R,R),
    getangle = cmd(W,R,R),
    getincangle = cmd(W,R,R),

    --- 3. Actors
    action = cmd(AC)
        / ACS":set_action(%1)",
    ai = cmd(AI)
        / ACS":set_ai(%1)",
    move = sp1 * t_move * (sp1 * t_define)^0
        / handle_move,

    cactor = cmd(D)
        / SPS":set_picnum(%1)",
    count = cmd(D)
        / ACS":set_count(%1)",
    cstator = cmd(D)
        / (SPS".cstat=_bit.bor(%1,"..SPS".cstat)"),
    cstat = cmd(D)
        / SPS".cstat=%1",
    clipdist = cmd(D)
        / SPS".clipdist=%1",
    sizeto = cmd(D,D)
        / "_con._sizeto(_aci,%1,%2)",  -- TODO: see control.lua:_sizeto
    sizeat = cmd(D,D)
        / (SPS".xrepeat,"..SPS".yrepeat=%1,%2"),
    strength = cmd(D)
        / SPS".extra=%1",
    addstrength = cmd(D)
        / (SPS".extra="..SPS".extra+%1"),
    spritepal = cmd(D),

    hitradius = cmd(D,D,D,D,D)
        / "_con._A_RadiusDamage(_aci,%1,%2,%3,%4,%5)",
    hitradiusvar = cmd(R,R,R,R,R),

    -- some commands taking read vars
    eshootvar = cmd(R),
    espawnvar = cmd(R),
    qspawnvar = cmd(R),
    eqspawnvar = cmd(R),
    operaterespawns = cmd(R),
    operatemasterswitches = cmd(R),
    checkactivatormotion = cmd(R),
    time = cmd(R),  -- no-op
    inittimer = cmd(R),
    lockplayer = cmd(R),
    shootvar = cmd(R),
    quake = cmd(R),
    jump = cmd(R),
    cmenu = cmd(R),
    soundvar = cmd(R),
    globalsoundvar = cmd(R),
    stopsoundvar = cmd(R),
    soundoncevar = cmd(R),
    angoffvar = cmd(R),
    checkavailweapon = cmd(R),
    checkavailinven = cmd(R),
    guniqhudid = cmd(R),
    savegamevar = cmd(R),
    readgamevar = cmd(R),
    userquote = cmd(R),
echo = cmd(D) / "_con._echo(%1)",  -- XXX: TEMP
--    echo = cmd(R),
    starttrackvar = cmd(R),
    clearmapstate = cmd(R),
    activatecheat = cmd(R),
    setgamepalette = cmd(R),

    -- some commands taking defines
    addammo = cmd(D,D)  -- NLCF
        / format("if (%s) then _con.longjmp() end", PLS":addammo(%1,%2)"),
    addweapon = cmd(D,D)  -- NLCF
        / format("if (%s) then _con.longjmp() end", PLS":addweapon(%1,%2)"),
    debris = cmd(D,D)
        / "_con._debris(_aci, %1, %2)",
    addinventory = cmd(D,D)
        / PLS":addinventory(%1,%2)",
    guts = cmd(D,D)
        / "_con._A_DoGuts(_aci,%1,%2)",

    -- cont'd
    addkills = cmd(D)
        / (PLS".actors_killed="..PLS".actors_killed+%1;"..ACS".actorstayput=-1"),
    addphealth = cmd(D)
        / format("_con._addphealth(%s,%s,%%1)", PLS"", SPS""),
    angoff = cmd(D)
        / "spritext[_aci].angoff=%1",
    debug = cmd(D)
        / handle_debug,
    endofgame = cmd(D)
        / "_con._endofgame(_pli,%1)",
    eqspawn = cmd(D),
    espawn = cmd(D),
    globalsound = cmd(D)
        / "_con._globalsound(_pli,%1)",
    lotsofglass = cmd(D)
        / "_con._A_SpawnGlass(_aci,%1)",
    mail = cmd(D)
        / "_con._spawnmany(_aci,4410,%1)",  -- TODO: dyntile
    money = cmd(D)
        / "_con._spawnmany(_aci,1233,%1)",  -- TODO: dyntile
    paper = cmd(D)
        / "_con._spawnmany(_aci,4460,%1)",  -- TODO: dyntile
    qspawn = cmd(D)
        / "_con.spawn(_aci,%1,true)",
    quote = cmd(D)
        / "_con._quote(_pli,%1)",
    savenn = cmd(D),
    save = cmd(D),
    sleeptime = cmd(D)
        / ACS".timetosleep=%1",
    soundonce = cmd(D)
        / "_con._soundonce(_aci,%1)",
    sound = cmd(D)
        / "_con._sound(_aci,%1)",
    spawn = cmd(D)
        / "_con.spawn(_aci,%1)",
    stopsound = cmd(D)
        / "_con._stopsound(_aci,%1)",

    eshoot = cmd(D),
    ezshoot = cmd(R,D),
    ezshootvar = cmd(R,R),
    shoot = cmd(D)
        / "_con._A_Shoot(_aci, %1)",
    zshoot = cmd(R,D),
    zshootvar = cmd(R,R),

    fall = cmd()
        / "_con._VM_FallSprite(_aci)",
    flash = cmd()
        / format("_con._flash(%s,%s)", SPS"", PLS""),
    getlastpal = cmd()
        / "_con._getlastpal(_aci)",
    insertspriteq = cmd()
        / "_con._addtodelqueue(_aci)",
    killit = cmd()  -- NLCF
        / "_con.killit()",
    mikesnd = cmd()
        / format("_con._soundonce(_aci,%s)", SPS".yvel"),
    nullop = cmd()
        / "",  -- NOTE: really generate no code
    pkick = cmd()
        / format("_con._pkick(%s,%s)", PLS"", ACS""),
    pstomp = cmd()
        / PLS":pstomp(_aci)",
    resetactioncount = cmd()
        / ACS":reset_acount()",
    resetcount = cmd()
        / ACS":set_count(0)",
    resetplayer = cmd()  -- NLCF
        / "if (_con._VM_ResetPlayer2(_pli,_aci)) then _con.longjmp() end",
    respawnhitag = cmd()
        / format("_con._respawnhitag(%s)", SPS""),
    tip = cmd()
        / PLS".tipincs=26",
    tossweapon = cmd()
        / "",  -- TODO
    wackplayer = cmd()
        / PLS":wack()",

    -- player/sprite searching
    findplayer = cmd(W),
    findotherplayer = cmd(W),
    findnearspritezvar = cmd(D,R,R,W),
    findnearspritez = cmd(D,D,D,W),
    findnearsprite3dvar = cmd(D,R,W),
    findnearsprite3d = cmd(D,D,W),
    findnearspritevar = cmd(D,R,W),
    findnearsprite = cmd(D,D,W),
    findnearactorzvar = cmd(D,R,R,W),
    findnearactorz = cmd(D,D,D,W),
    findnearactor3dvar = cmd(D,R,W),
    findnearactor3d = cmd(D,D,W),
    findnearactorvar = cmd(D,R,W),
    findnearactor = cmd(D,D,W),

    -- quotes
    qsprintf = sp1 * t_rvar * sp1 * t_rvar * (sp1 * t_rvar)^-32,
    qgetsysstr = cmd(R,R),
    qstrcat = cmd(R,R),
    qstrcpy = cmd(R,R),
    qstrlen = cmd(R,R),
    qstrncat = cmd(R,R),
    qsubstr = cmd(R,R),

    -- array stuff
    copy = sp1 * t_identifier * arraypat * sp1 * t_identifier * arraypat * sp1 * t_rvar,
    setarray = sp1 * t_identifier * arraypat * sp1 * t_rvar,

    activatebysector = cmd(R,R),
    addlogvar = cmd(R),
    addlog = cmd() * #sp1,
    addweaponvar = cmd(R,R),  -- exec SPECIAL HANDLING!
    cansee = cmd(R,R,R,R,R,R,R,R,W),
    canseespr = cmd(R,R,W),
    changespritesect = cmd(R,R),
    changespritestat = cmd(R,R),
    clipmove = cmd(W,W,W,R,W,R,R,R,R,R,R),
    clipmovenoslide = cmd(W,W,W,R,W,R,R,R,R,R,R),
    displayrand = cmd(W),
    displayrandvar = cmd(W,D),
    displayrandvarvar = cmd(W,R),
    dist = cmd(W,R,R),
    dragpoint = cmd(R,R,R),
    hitscan = cmd(R,R,R,R,R,R,R,W,W,W,W,W,W,R), -- 7R 6W 1R

    -- screen text and numbers display
    gametext = cmd(R,R,R,R,R,R,R,R,R,R,R),  -- 11 R
    gametextz = cmd(R,R,R,R,R,R,R,R,R,R,R,R),  -- 12 R
    digitalnumber = cmd(R,R,R,R,R,R,R,R,R,R,R),  -- 11R
    digitalnumberz = cmd(R,R,R,R,R,R,R,R,R,R,R,R),  -- 12R
    minitext = cmd(R,R,R,R,R),

    ldist = cmd(W,R,R),
    lineintersect = cmd(R,R,R,R,R,R,R,R,R,R,W,W,W,W),  -- 10R 4W
    rayintersect = cmd(R,R,R,R,R,R,R,R,R,R,W,W,W,W),  -- 10R 4W
    loadmapstate = cmd(),
    savemapstate = cmd(),
    movesprite = cmd(R,R,R,R,R,W),
    neartag = cmd(R,R,R,R,R,W,W,W,W,R,R),
    operateactivators = cmd(R,R),
    operatesectors = cmd(R,R),
    palfrom = (sp1 * t_define)^-4
        / handle_palfrom,

    operate = cmd() * #sp1
        / "_con._operate(_aci)",

    myos = cmd(R,R,R,R,R),
    myosx = cmd(R,R,R,R,R),
    myospal = cmd(R,R,R,R,R,R),
    myospalx = cmd(R,R,R,R,R,R),

    headspritesect = cmd(R,R),
    headspritestat = cmd(R,R),
    nextspritesect = cmd(R,R),
    nextspritestat = cmd(R,R),
    prevspritesect = cmd(R,R),
    prevspritestat = cmd(R,R),

    readarrayfromfile = cmd(I,D),
    writearraytofile = cmd(I,D),

    redefinequote = sp1 * t_define * newline_term_string
        / function(qnum, qstr) return format("_con._definequote(%d,%q)", qnum, stripws(qstr)) end,
    resizearray = cmd(I,R),
    getarraysize = cmd(I,W),
    rotatepoint = cmd(R,R,R,R,R,W,W),
    rotatesprite = cmd(R,R,R,R,R,R,R,R,R,R,R,R),  -- 12R
    rotatesprite16 = cmd(R,R,R,R,R,R,R,R,R,R,R,R),  -- 12R
    sectorofwall = cmd(W,R,R),
    sectclearinterpolation = cmd(R),
    sectsetinterpolation = cmd(R),

    sectgethitag = cmd(),
    sectgetlotag = cmd(),
    spgethitag = cmd(),
    spgetlotag = cmd(),

    showview = cmd(R,R,R,R,R,R,R,R,R,R),  -- 10R
    showviewunbiased = cmd(R,R,R,R,R,R,R,R,R,R),  -- 10R
    smaxammo = cmd(R,R),
    gmaxammo = cmd(R,W),
    spriteflags = cmd(R),  -- also see outer
    ssp = cmd(R,R),
    startlevel = cmd(R,R),
    starttrack = cmd(D),
    stopactorsound = cmd(R,R),
    stopallsounds = cmd(),
    updatesector = cmd(R,R,W),
    updatesectorz = cmd(R,R,R,W),

    getactorangle = cmd(W),
    setactorangle = cmd(R),
    getplayerangle = cmd(W),
    setplayerangle = cmd(R),
    getangletotarget = cmd(W),

    getceilzofslope = cmd(R,R,R,W),
    getflorzofslope = cmd(R,R,R,W),
    getcurraddress = cmd(W),  -- XXX
    getkeyname = cmd(R,R,R),
    getpname = cmd(R,R),
    gettextureceiling = cmd(),
    gettexturefloor = cmd(),
    getticks = cmd(W),
    gettimedate = cmd(W,W,W,W,W,W,W,W),
    getzrange = cmd(R,R,R,R,W,W,W,W,R,R),

    setactorsoundpitch = cmd(R,R,R),
    setaspect = cmd(R,R),
}

local Cif = {
    ifai = cmd(AI)
        / ACS":has_ai(%1)",
    ifaction = cmd(AC)
        / ACS":has_action(%1)",
    ifmove = cmd(MV)
        / ACS":has_move(%1)",

    ifrnd = cmd(D)
        / "_con.rnd(%1)",
    ifpdistl = cmd(D)
        / function(val) return "_dist<"..val, "", "_con._sleepcheck(_aci,_dist)" end,
    ifpdistg = cmd(D)
        / function(val) return "_dist>"..val, "", "_con._sleepcheck(_aci,_dist)" end,
    ifactioncount = cmd(D)
        / ACS":get_acount()>=%1",
    ifcount = cmd(D)
        / ACS":get_count()>=%1",
    ifactor = cmd(D)
        / SPS".picnum==%1",
    ifstrength = cmd(D)
        / SPS".extra<=%1",
    ifspawnedby = cmd(D)
        / ACS".picnum==%1",
    ifwasweapon = cmd(D)
        / ACS".picnum==%1",
    ifgapzl = cmd(D)  -- factor into a con.* function?
        / format("_bit.arshift(%s-%s,8)<%%1", ACS".floorz", ACS".ceilingz"),
    iffloordistl = cmd(D)
        / format("(%s-%s)<=256*%%1", ACS".floorz", SPS".z"),
    ifceilingdistl = cmd(D)
        / format("(%s-%s)<=256*%%1", SPS".z", ACS".ceilingz"),
    ifphealthl = cmd(D)
        / format("sprite[%s].extra<%%1", PLS".i"),
    ifspritepal = cmd(D)
        / SPS".pal==%1",
    ifgotweaponce = cmd(D)
        / "false",  -- TODO? (multiplayer only)
    ifangdiffl = cmd(D)
        / format("_con._angdiffabs(%s,%s)<=%%1", PLS".ang", SPS".ang"),
    ifsound = cmd(D)
        / "",
    -- vvv TODO: this is not correct for GET_ACCESS or GET_SHIELD.
    ifpinventory = cmd(D,D)
        / format("_con._getinventory(%s,%%1,_aci)~=%%2", PLS""),

    ifvarl = cmd(R,D),
    ifvarg = cmd(R,D),
    ifvare = cmd(R,D)
        / "%1==%2",
    ifvarn = cmd(R,D),
    ifvarand = cmd(R,D),
    ifvaror = cmd(R,D),
    ifvarxor = cmd(R,D),
    ifvareither = cmd(R,D),

    ifvarvarg = cmd(R,R),
    ifvarvarl = cmd(R,R),
    ifvarvare = cmd(R,R),
    ifvarvarn = cmd(R,R),
    ifvarvarand = cmd(R,R),
    ifvarvaror = cmd(R,R),
    ifvarvarxor = cmd(R,R),
    ifvarvareither = cmd(R,R),

    ifactorsound = cmd(R,R),

    ifp = (sp1 * t_define)^1
        / function (...) return format("_con._ifp(%d,_pli,_aci)", bit.bor(...)) end,
    ifsquished = cmd()
        / "false",  -- TODO
    ifserver = cmd(),
    ifrespawn = cmd()
        / format("_con._checkrespawn(%s)", SPS""),
    ifoutside = cmd()
        / format("_bit.band(sector[%s].ceilingstat,1)~=0", SPS".sectnum"),
    ifonwater = cmd()
        / format("sector[%s].lotag==1 and _math.abs(%s-sector[%s].floorz)<32*256",
                 SPS".sectnum", SPS".z", SPS".sectnum"),
    ifnotmoving = cmd()
        / "_bit.band(actor[_aci].movflag,49152)>16384",
    ifnosounds = cmd()
        / "not _con._ianysound()",
    ifmultiplayer = cmd()
        / "false",  -- TODO?
    ifinwater = cmd()
        / format("sector[%s].lotag==2", SPS".sectnum"),
    ifinspace = cmd()
        / format("_con._checkspace(%s,false)", SPS".sectnum"),
    ifinouterspace = cmd()
        / format("_con._checkspace(%s,true)", SPS".sectnum"),
    ifhitweapon = cmd()
        / "_con._A_IncurDamage(_aci)>=0",
    ifhitspace = cmd()
        / "_con._testkey(_pli,29)",  -- XXX
    ifdead = cmd()
        / SPS".extra<=0",
    ifclient = cmd(),
    ifcanshoottarget = cmd()
        / "_con._canshoottarget(_dist,_aci)",
    ifcanseetarget = cmd()  -- XXX: 1536 is SLEEPTIME
        / function() return format("_con._canseetarget(%s,%s)", SPS"", PLS""), ACS".timetosleep=1536" end,
    ifcansee = cmd() * #sp1
        / format("_con._cansee(_aci,%s)", PLS""),
    ifbulletnear = cmd()
        / "_con._bulletnear(_aci)",
    ifawayfromwall = cmd()
        / format("_con._awayfromwall(%s,108)", SPS""),
    ifactornotstayput = cmd()
        / ACS".actorstayput==-1",
}


----==== Tracing and reporting ====----

-- g_newlineidxs will contain the 1-based file offsets to "\n" characters
local g_newlineidxs = {}

-- Returns index into the sorted table tab such that
--   tab[index] <= searchelt < tab[index+1].
-- Preconditions:
--  tab[i] < tab[i+1] for 0 <= i < #tab
--  tab[0] <= searchelt < tab[#tab]
-- If #tab is less than 2, returns 0. This plays nicely with newline index
-- tables like { [0]=0, [1]=len+1 }, e.g. if the file doesn't contain any.
local function bsearch(tab, searchelt)
--    printf("bsearch(tab, %d)", searchelt)
    local l, r = 0, #tab
    local i

    if (r < 2) then
        return 0
    end

    while (l ~= r) do
        i = l + math.ceil((r-l)/2)  -- l < i <= r
        assert(l < i and i <= r)
        local elt = tab[i]
--        printf("l=%d tab[%d]=%d r=%d", l, i, elt, r)

        if (searchelt == elt) then
            return i
        end

        if (searchelt < elt) then
            r = i-1
        else  -- (searchelt > elt)
            l = i
        end
    end

--    printf("return tab[%d]=%d", l, tab[l])
    return l
end

function getlinecol(pos)  -- local
    assert(type(pos)=="number")
    local line = bsearch(g_newlineidxs, pos)
    assert(line and g_newlineidxs[line]<=pos and pos<g_newlineidxs[line+1])
    local col = pos-g_newlineidxs[line]
    return line+1, col-1
end

-- A generic trace function, prints a position together with the match content.
-- The 'doit' parameter can be used to temporarily enable/disable a particular
-- tracing function.
local function TraceFunc(pat, label, doit)
    assert(doit ~= nil)
    pat = Pat(pat)

    if (doit) then
        local function tfunc(subj, pos, a)
            printf("%s:%s: %s", linecolstr(pos), label, a)
            return true
        end
        pat = lpeg.Cmt(pat, tfunc)
    elseif (label=="kw") then  -- HACK
        local function tfunc(subj, pos, a)
            g_lastkwpos = pos
            g_lastkw = a
            return true
        end
        -- XXX: is there a better way?
        pat = lpeg.Cmt(pat, tfunc)
    end

    return pat
end

local function BadIdent(pat)
    local function tfunc(subj, pos, a)
        if (g_warn["bad-identifier"] and not g_badids[a]) then
            warnprintf("bad identifier: %s", a)
            g_badids[a] = true
        end
        return true
    end
    return lpeg.Cmt(Pat(pat), tfunc)
end

-- These are tracers for specific patterns which can be disabled
-- if desired.
local function Keyw(kwname) return TraceFunc(kwname, "kw", false) end
local function NotKeyw(text) return TraceFunc(text, "!kw", false) end
local function Ident(idname) return TraceFunc(idname, "id", false) end
local function Stmt(cmdpat) return TraceFunc(cmdpat, "st", false) end

--local function Temp(kwname) return TraceFunc(kwname, "temp", true) end
--Ci["myosx"] = Temp(Ci["myosx"])

----==== Translator continued ====----
local function after_inner_cmd_Cmt(subj, pos, ...)
    local capts = {...}

    if (g_numerrors == inf) then
        return nil
    end

    if (type(capts[1])=="string" and capts[2]==nil) then
        return true, capts[1] .."--"..linecolstr(pos) --TEMP
    end

    return true
end

local function after_if_cmd_Cmt(subj, pos, ...)
    local capts = {...}

    if (g_numerrors == inf) then
        return nil
    end

    if (capts[1] ~= nil) then
        assert(#capts <= 3)
        for i=1,#capts do
            assert(type(capts[i]=="string"))
        end
        return true, unpack(capts, 1, #capts)
    end

    return true
end

local function after_cmd_Cmt(subj, pos, ...)
    if (g_numerrors == inf) then
--        print("Aborting parsing...")
        return nil  -- make the match fail, bail out of parsing
    end

    return true  -- don't return any captures
end

-- attach the command names at the front!
local function attachnames(kwtab, matchtimefunc)
    for cmdname,cmdpat in pairs(kwtab) do
        -- The match-time function capture at the end is so that every
        -- command acts as a barrier to captures to prevent stack overflow (and
        -- to make lpeg.match return a subject position at the end)
        kwtab[cmdname] = lpeg.Cmt(Keyw(cmdname) * cmdpat, matchtimefunc)
    end
end

attachnames(Co, after_cmd_Cmt)
attachnames(Ci, after_inner_cmd_Cmt)
attachnames(Cif, after_if_cmd_Cmt)


-- Takes one or more tables and +'s all its patterns together in reverse
-- lexicographical order.
-- Each such PATTAB must be a table that maps command names to their patterns.
local function all_alt_pattern(...)
    local cmds = {}

    local args = {...}
    assert(#args <= 2)

    for argi=1,#args do
        local pattab = args[argi]

        -- pairs() iterates in undefined order, so we first fill in the names...
        for cmdname,_ in pairs(pattab) do
            cmds[#cmds+1] = cmdname
        end
    end

    -- ...and then sort them in ascending lexicographical order
    table.sort(cmds)

    local pat = Pat(false)

    for i=1,#cmds do
        local ourpat = args[1][cmds[i]] or args[2][cmds[i]]
        -- shorter commands go at the end!
        pat = pat + ourpat
    end

    return pat
end

-- actor ORGANTIC is greeting!
local function warn_on_lonely_else(pos)
    pwarnprintf(pos, "found `else' with no `if'")
end

local con_inner_command = all_alt_pattern(Ci)
local con_if_begs = all_alt_pattern(Cif)

local lone_else = (POS() * "else" * sp1)/warn_on_lonely_else

local stmt_list = Var("stmt_list")
-- possibly empty statement list:
local stmt_list_or_eps = lpeg.Ct((stmt_list * sp1)^-1)
local stmt_list_nosp_or_eps = (stmt_list * (sp1 * stmt_list)^0)^-1

-- common to actor and useractor: <name/tilenum> [<strength> [<action> [<move> [<flags>... ]]]]
local common_actor_end = sp1 * lpeg.Ct(t_define *
    (sp1 * t_define *
     (sp1 * t_action *
      (sp1 * t_move *
       (sp1 * t_define)^0
      )^-1
     )^-1
    )^-1)
* sp1 * stmt_list_or_eps * "enda"

--== block delimiters (no recursion) ==--
local Cb = {
    -- actor (...)
    actor = lpeg.Cc(nil) * common_actor_end / on_actor_end,
    -- useractor <actortype> (...)
    useractor = sp1 * t_define * common_actor_end / on_actor_end,
    -- eventloadactor <name/tilenum>
    eventloadactor = lpeg.Cc(nil) * sp1 * lpeg.Ct(t_define)
        * sp1 * stmt_list_or_eps * "enda" / on_actor_end,

    onevent = sp1 * t_define * sp1 * stmt_list_or_eps * "endevent"
        / on_event_end,

    state = sp1 * t_identifier * sp1 * stmt_list_or_eps * "ends"
        / on_state_end,
}

attachnames(Cb, after_cmd_Cmt)


local t_good_identifier = Range("AZ", "az", "__") * Range("AZ", "az", "__", "09")^0

-- CON isaltok also has chars in "{}.", but these could potentially
-- interfere with *CON* syntax.  The "]" is so that the number in e.g. array[80]
-- isn't considered a broken identifier.
-- "-" is somewhat problematic, but we allow it only as 2nd and up character, so
-- there's no ambiguity with unary minus.  (Commands must be separated by spaces
-- in CON, so a trailing "-" is "OK", too.)
-- This is broken in itself, so we ought to make a compatibility/modern CON switch.
local t_broken_identifier = BadIdent(-((t_number + t_good_identifier) * (sp1 + Set("[]:"))) *
                                     (alphanum + Set("_/\\*?")) * (alphanum + Set("_/\\*-+?"))^0)

-- These two tables hold code to be inserted at a later point: either at
-- the end of the "if" body, or the end of the whole "if [else]" block.
-- For CON interpreter patterns like these:
--  VM_CONDITIONAL(<condition>);
--  <do_something_afterwards>
-- (Still not the same if the body returns or jumps out)
local g_endIfCode = {}
local g_endIfElseCode = {}

local function add_deferred_code(tab, lev, str)
    if (str ~= nil) then
        assert(type(str)=="string")
        tab[lev] = str
    end
end

local function get_deferred_code(tab, lev, code)
    if (tab[lev]) then
        code = code..tab[lev]
        tab[lev] = nil
    end
    return code
end

local function begin_if_fn(condstr, endifstr, endifelsestr)
    condstr = condstr or "TODO"
    assert(type(condstr)=="string")

    add_deferred_code(g_endIfCode, g_iflevel, endifstr)
    add_deferred_code(g_endIfElseCode, g_ifelselevel, endifelsestr)

    g_iflevel = g_iflevel+1
    g_ifelselevel = g_ifelselevel+1

    return format("if (%s) then", condstr)
end

local function end_if_fn()
    g_iflevel = g_iflevel-1
    local code = get_deferred_code(g_endIfCode, g_iflevel, "")
    if (code ~= "") then
        return code
    end
end

local function end_if_else_fn()
    g_ifelselevel = g_ifelselevel-1
    return get_deferred_code(g_endIfElseCode, g_ifelselevel, "end ")
end

local function check_else_Cmt()
    -- match an 'else' only at the outermost level
    -- XXX: THIS IS STILL WRONG
    local good = (g_iflevel==0)
    if (good) then
        return true, "else"
    end

    -- return nothing, making the Cmt fail
end

--- The final grammar!
local Grammar = Pat{
    -- The starting symbol.
    -- A translation unit is a (possibly empty) sequence of outer CON
    -- commands, separated by at least one whitespace which may be
    -- omitted at the EOF.
    sp0 * (all_alt_pattern(Co, Cb) * sp1)^0,

    -- Some often-used terminals follow.  These appear here because we're
    -- hitting a limit with LPeg else.
    -- http://lua-users.org/lists/lua-l/2008-11/msg00462.html

    -- NOTE: NW demo (NWSNOW.CON) contains a Ctrl-Z char (decimal 26)
    whitespace = Set(" \t\r\26") + newline + Set("(),;") + comment + linecomment,

    t_identifier_all = t_broken_identifier + t_good_identifier,
    -- NOTE: -conl.keyword alone would be wrong, e.g. "state breakobject":
    -- NOTE 2: The + "[" is so that stuff like
    --   getactor[THISACTOR].x x
    --   getactor[THISACTOR].y y
    -- is parsed correctly.  (Compared with this:)
    --   getactor[THISACTOR].x x
    --   getactor [THISACTOR].y y
    -- This is in need of cleanup!
    t_identifier = -NotKeyw(conl.keyword * (sp1 + "[")) * lpeg.C(t_identifier_all),
    t_define = (POS() * lpeg.C(t_maybe_minus) * t_identifier / lookup_defined_label) + t_number,

    t_move =
        POS()*t_identifier / function(...) return lookup_composite(LABEL.MOVE, ...) end +
        POS()*t_number / function(...) return check_composite_literal(LABEL.MOVE, ...) end,

    t_ai =
        POS()*t_identifier / function(...) return lookup_composite(LABEL.AI, ...) end +
        POS()*t_number / function(...) return check_composite_literal(LABEL.AI, ...) end,

    t_action =
        POS()*t_identifier / function(...) return lookup_composite(LABEL.ACTION, ...) end +
        POS()*t_number / function(...) return check_composite_literal(LABEL.ACTION, ...) end,

    t_arrayexp = t_identifier * arraypat * memberpat^-1,

    -- SWITCH
    switch_stmt = Keyw("switch") * sp1 * t_rvar *
        (Var("case_block") + Var("default_block"))^0 * sp1 * "endswitch",

    -- NOTE: some old DNWMD has "case: PIGCOP".  I don't think I'll allow that.
    case_block = (sp1 * Keyw("case") * sp1 * t_define/"XXX_CASE" * (sp0*":")^-1)^1 * sp1 *
        stmt_list_nosp_or_eps, -- * "break",

    default_block = sp1 * Keyw("default") * (sp0*":"*sp0 + sp1) * stmt_list_nosp_or_eps,  -- * "break",

    if_stmt = con_if_begs/begin_if_fn * sp1
        * Var("single_stmt") * (Pat("")/end_if_fn)
        * (sp1 * lpeg.Cmt(Pat("else"), check_else_Cmt) * sp1 * Var("single_stmt"))^-1
        * (Pat("")/end_if_else_fn),

    -- TODO?: SST TC has "state ... else ends"
    while_stmt = Keyw("whilevarvarn") * sp1 * t_rvar * sp1 * t_rvar * sp1 * Var("single_stmt")
        + Keyw("whilevarn") * sp1 * t_rvar * sp1 * t_define/"WHILE_XXX" * sp1 * Var("single_stmt"),

    -- TODO: some sp1 --> sp0?
    single_stmt = Stmt(
        lone_else^-1 *
            ( Keyw("{") * sp1 * "}"  -- space separation of commands in CON is for a reason!
              + Keyw("{") * sp1 * stmt_list * sp1 * "}"
              + (con_inner_command + Var("switch_stmt") + Var("if_stmt") + Var("while_stmt"))
--              + lpeg.Cmt(t_newline_term_str, function (subj, curpos) print("Error at "..curpos) end)
            )),

    -- a non-empty statement/command list
    stmt_list = Var("single_stmt") * (sp1 * Var("single_stmt"))^0,
}


local function setup_newlineidxs(contents)
    local newlineidxs = {}
    for i in string.gmatch(contents, "()\n") do
        newlineidxs[#newlineidxs+1] = i
    end
    if (#newlineidxs == 0) then
        -- try CR only (old Mac)
        for i in string.gmatch(contents, "()\r") do
            newlineidxs[#newlineidxs+1] = i
        end
--        if (#newlineidxs > 0) then print('CR-only lineends detected.') end
    end
    -- dummy newlines at beginning and end
    newlineidxs[#newlineidxs+1] = #contents+1
    newlineidxs[0] = 0

    return newlineidxs
end

---
local function do_flatten_codetab(code, intotab)
    for i=1,math.huge do
        local elt = code[i]

        if (type(elt)=="string") then
            intotab[#intotab+1] = elt
        elseif (type(elt)=="table") then
            do_flatten_codetab(elt, intotab)
        else
            assert(elt==nil)
            return
        end
    end
end

-- Return a "string buffer" table that can be table.concat'ed
-- to get the code string.
local function flatten_codetab(codetab)
    local tmpcode = {}
    do_flatten_codetab(codetab, tmpcode)
    return tmpcode
end

local function get_code_string(codetab)
    return table.concat(flatten_codetab(g_curcode), "\n")
end


---=== EXPORTED FUNCTIONS ===---

local g_printcode = true

function parse(contents)  -- local
    -- save outer state
    local lastkw, lastkwpos, numerrors = g_lastkw, g_lastkwpos, g_numerrors
    local newlineidxs = g_newlineidxs

    g_iflevel = 0
    g_ifelselevel = 0
    g_have_file[g_filename] = true

    -- set up new state
    -- TODO: pack into one "parser state" table?
    g_lastkw, g_lastkwpos, g_numerrors = nil, nil, 0
    g_newlineidxs = setup_newlineidxs(contents)

    g_recurslevel = g_recurslevel+1

    addcodef("-- BEGIN %s", g_filename)

    local idx = lpeg.match(Grammar, contents)

    if (not idx) then
        printf("[%d] Match failed.", g_recurslevel)
        g_numerrors = inf
    elseif (idx == #contents+1) then
        if (g_numerrors ~= 0) then
            printf("[%d] Matched whole contents (%d errors).",
                   g_recurslevel, g_numerrors)
        elseif (g_recurslevel==0) then
            printf("[0] Matched whole contents.")
        end
    else
        local i, col = getlinecol(idx)
        local bi, ei = g_newlineidxs[i-1]+1, g_newlineidxs[i]-1

        printf("[%d] Match succeeded up to line %d, col %d (pos=%d, len=%d)",
               g_recurslevel, i, col, idx, #contents)
        g_numerrors = inf

--        printf("Line goes from %d to %d", bi, ei)
        local suffix = ""
        if (ei-bi > 76) then
            ei = bi+76
            suffix = " (...)"
        end
        printf("%s%s", string.sub(contents, bi, ei), suffix)

        if (g_lastkwpos) then
            i, col = getlinecol(g_lastkwpos)
            printf("Last keyword was at line %d, col %d: %s", i, col, g_lastkw)
        end
    end

    addcodef("-- END %s", g_filename)
    g_recurslevel = g_recurslevel-1

    -- restore outer state
    g_lastkw, g_lastkwpos = lastkw, lastkwpos
    g_numerrors = (g_numerrors==inf and inf) or numerrors
    g_newlineidxs = newlineidxs
end

local function handle_cmdline_arg(str)
    if (str:sub(1,1)=="-") then
        if (#str == 1) then
            printf("Warning: input from stdin not supported")
        else
            local ok = false
            local kind = str:sub(2,2)

            if (kind=="W" and #str >= 3) then
                -- warnings
                local val = true
                local warnstr = str:sub(3)

                if (#warnstr >= 4 and warnstr:sub(1,3)=="no-") then
                    val = false
                    warnstr = warnstr:sub(4)
                end

                if (type(g_warn[warnstr])=="boolean") then
                    g_warn[warnstr] = val
                    ok = true
                end
            end

            if (not ok) then
                printf("Warning: Unrecognized option %s", str)
            end
        end

        return true
    end
end

local function reset_all()
    reset_labels()
    reset_gamedata()
    reset_codegen()
end

local function print_on_failure(msg)
    if (g_lastkwpos ~= nil) then
        printf("LAST KEYWORD POSITION: %s, %s", linecolstr(g_lastkwpos), g_lastkw)
    end
    print(msg)
end

if (string.dump) then
    -- running stand-alone

    local i = 1
    while (arg[i]) do
        if (handle_cmdline_arg(arg[i])) then
            table.remove(arg, i)
        else
            i = i+1
        end
    end

    for argi=1,#arg do
        local filename = arg[argi]

        reset_all()

        g_directory = filename:match("(.*/)") or ""
        filename = filename:sub(#g_directory+1, -1)

        -- NOTE: xpcall isn't useful here since the traceback won't give us
        -- anything inner to the lpeg.match call
        local ok, msg = pcall(do_include_file, g_directory, filename)
        -- ^v Swap commenting (comment top, uncomment bottom line) to get backtraces
--        local ok, msg = true, do_include_file(g_directory, filename)

        if (not ok) then
            print_on_failure(msg)
        end

        if (g_printcode) then
            local file = require("io").stderr
            file:write(format("-- GENERATED CODE for \"%s\":\n", filename))
            file:write(get_code_string(g_curcode))
            file:write("\n")
        end
    end
else
    -- running from EDuke32
    function compile(filename)
        -- TODO: pathsearchmode=1 set in G_CompileScripts
        reset_all()
        local ok, msg = pcall(do_include_file, "", filename)
        if (not ok) then
            print_on_failure(msg)
            return nil
        else
            return get_code_string(g_curcode)
        end
    end
end
