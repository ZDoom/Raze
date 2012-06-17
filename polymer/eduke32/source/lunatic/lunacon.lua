-- LunaCON CON to Lunatic translator
-- requires LPeg, http://www.inf.puc-rio.br/~roberto/lpeg/lpeg.html

local lpeg = require("lpeg")

if (not _EDUKE32_LUNATIC) then
    require("strict")
end


-- I think that the "too many pending calls/choices" is unavoidable in general.
-- This limit is of course still arbitrary, but writing long if/else cascades
-- in CON isn't pretty either (though sometimes necessary because nested switches
-- don't work?)
-- See also:  http://lua-users.org/lists/lua-l/2010-03/msg00086.html
lpeg.setmaxstack(1024);


local Pat, Set, Range, Var = lpeg.P, lpeg.S, lpeg.R, lpeg.V


---- All keywords pattern -- needed for CON syntax
local con = require("con_lang")


local function match_until(matchsp, untilsp)  -- (!untilsp matchsp)* in PEG
    -- sp: string or pattern
    return (matchsp - Pat(untilsp))^0
end

local function printf(fmt, ...)
    print(string.format(fmt, ...))
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
local g_numerrors = 0

local g_ifnestlevel = 0  -- needed to cope with CONs dangling-else resolution


local function getlinecol(pos) end -- fwd-decl

local function linecolstr(pos)
    local line, col = getlinecol(pos)
    return string.format("%d:%d", line, col)
end

local function errprintf(fmt, ...)
    if (g_lastkwpos) then
        printf("%s %s: error: "..fmt, g_filename, linecolstr(g_lastkwpos), ...)
    else
        printf("%s ???: error: "..fmt, g_filename, ...)
    end
    g_numerrors = g_numerrors+1
end

local function warnprintf(fmt, ...)
    if (g_lastkwpos) then
        printf("%s %s: warning: "..fmt, g_filename, linecolstr(g_lastkwpos), ...)
    else
        printf("%s ???: warning: "..fmt, g_filename, ...)
    end
end

local function parse_number(numstr)
    local num = tonumber(numstr)

    -- TODO: print line number
    if (num < -0x80000000 or num > 0xffffffff) then
        errprintf("number %s out of the range of a 32-bit integer", numstr)
        num = NaN
    elseif (num >= 0x80000000 and numstr:sub(1,2):lower()~="0x") then
        warnprintf("number %s converted to a negative one", numstr)
        num = num-0x100000000
    end

    return num
end


local LABEL_DEFINE = 1
--local LABEL_STATE  = 2
--local LABEL_ACTOR  = 4
local LABEL_ACTION = 8
local LABEL_AI     = 16
local LABEL_MOVE   = 32

local g_labeldef = {}
local g_labeltype = {}

local function reset_labels()
    g_labeldef = { NO=0 }
    g_labeltype = { NO=LABEL_DEFINE+LABEL_ACTION+LABEL_AI+LABEL_MOVE }

    for i=1,#con.labels do
        for label, val in pairs(con.labels[i]) do
            g_labeldef[label] = val
            g_labeltype[label] = LABEL_DEFINE
        end
    end
end

local function lookup_defined_label(identifier)
    local num = g_labeldef[identifier]

    if (num == nil) then
        errprintf("label \"%s\" is not defined", identifier)
        return -inf  -- return a number for type cleanness
    end

    return num
end

local function do_define_label(identifier, idornum)
    -- TODO: label types
    local num

    if (type(idornum)=="number") then
        num = idornum
    else
        assert(idornum ~= nil)
        num = lookup_defined_label(idornum)
        if (num == -inf) then
            return
        end
    end

    local oldnum = g_labeldef[identifier]
    if (oldnum) then
        -- con.labels[...]: don't warn for wrong PROJ_ redefinitions
        if (oldnum ~= num and con.labels[2][identifier]==nil) then
            warnprintf("label \"%s\" not redefined with new value %d (old: %d)",
                       identifier, num, oldnum)
        end
        return
    end

    g_labeldef[identifier] = num
end

local function parse(contents) end -- fwd-decl

local function do_include_file(dirname, filename) end
local function cmd_include(filename) end

if (_EDUKE32_LUNATIC) then
    -- NOT IMPLEMENTED
else
    function do_include_file(dirname, filename)
        local io = require("io")

        local fd, msg = io.open(dirname..filename)
        if (fd == nil) then
            -- strip up to and including first slash:
            filename = string.gsub(filename, "^.-/", "")
            fd, msg = io.open(dirname..filename)

            if (fd == nil) then
                printf("Fatal error: couldn't open %s", msg)
                g_numerrors = inf
                return
            end
        end

        printf("%s[%d] Parsing file \"%s\"", (g_recurslevel==-1 and "\n---- ") or "",
               g_recurslevel+1, dirname..filename);

        local contents = fd:read("*all")
        fd:close()

        if (contents == nil) then
            -- maybe that file name turned out to be a directory or other
            -- special file accidentally
            printf("Fatal error: couldn't read from \"%s\"", dirname..filename)
            g_numerrors = inf
            return
        end

        local oldfilename = g_filename
        g_filename = filename
        parse(contents)
        g_filename = oldfilename
    end

    function cmd_include(filename)
        do_include_file(g_directory, filename)
    end
end

--- Per-module game data
local g_data = {}
local EPMUL = con.MAXLEVELS

local function reset_gamedata()
    g_data = {}

    -- [EPMUL*ep + lev] = { ptime=<num>, dtime=<num>, fn=<str>, name=<str> }
    g_data.levels = {}
end

local function cmd_definelevelname(vol, lev, fn, ptstr, dtstr, levname)
    if (vol < 0 or vol >= con.MAXVOLUMES) then
        errprintf("volume number exceeds maximum volume count.")
    end

    if (lev < 0 or lev >= con.MAXLEVELS) then
        errprintf("level number exceeds maximum number of levels per episode.")
    end

    -- TODO: Bcorrectfilename(fn)

    local function secs(tstr)
        local m, s = string.match(tstr, ".+:.+")
        m, s = tonumber(m), tonumber(s)
        return (m and s) and m*60+s or 0
    end

    g_data.levels[EPMUL*vol+lev] = {
        ptime=secs(ptstr), dtime=secs(dtstr), fn=fn, name=levname
    }
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
local t_number = lpeg.C(
    t_maybe_minus * ((Pat("0x") + "0X")*Range("09", "af", "AF")^1 + Range("09")^1)
                       ) / parse_number
-- Valid identifier names are disjunct from keywords!
-- XXX: CON is more permissive with identifier name characters:
local t_identifier = Var("t_identifier")
-- This one matches keywords, too:
local t_identifier_all = Var("t_identifier_all")
local t_define = Var("t_define")
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


local D, R, W, I = -1, -2, -3, -4;

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
        assert(vartypes[i] < 0)

        if (vartypes[i] == D) then
            pat = pat * sp1 * t_define
        elseif (vartypes[i] == R) then
            pat = pat * sp1 * t_rvar
        elseif (vartypes[i] == W) then
            pat = pat * sp1 * t_wvar
        else  -- I
            assert(vartypes[i] == I)
            pat = pat * sp1 * t_identifier
        end
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
    scriptsize = cmd(D),  -- unused
    cheatkeys = cmd(D,D),

    definecheat = newline_term_string,  -- XXX: actually tricker syntax (TS)
    definegamefuncname = newline_term_string,  -- XXX: TS?
    definegametype = n_defines(2) * newline_term_string,
    definelevelname = n_defines(2) * sp1 * t_filename * sp1 * t_time * sp1 * t_time *
        newline_term_string / cmd_definelevelname,
    defineskillname = sp1 * t_define * newline_term_string,
    definevolumename = sp1 * t_define * newline_term_string,

    definequote = sp1 * t_define * newline_term_string,
    defineprojectile = cmd(D,D,D),
    definesound = sp1 * t_define * sp1 * maybe_quoted_filename * n_defines(5),  -- XXX: TS

    -- NOTE: gamevar.ogg and the like is OK, too
    music = sp1 * t_define * match_until(sp1 * t_filename, sp1 * con.keyword * sp1),

    --- 3. Game Settings
    -- gamestartup has 25/29 fixed defines, depending on 1.3D/1.5 version:
    gamestartup = (sp1 * t_define)^25,
    spritenopal = cmd(D),
    spritenoshade = cmd(D),
    spritenvg = cmd(D),
    spriteshadow = cmd(D),

    spriteflags = cmd(D,D),  -- also see inner

    --- 4. Game Variables / Arrays
    gamevar = cmd(I,D,D),
    gamearray = cmd(I,D),

    --- 5. Top level commands that are also run-time commands
    action = sp1 * t_identifier * (sp1 * t_define)^-5 / function(id) do_define_label(id, 0) end,  -- TEMP
    ai = sp1 * t_identifier * (sp1 * t_define)^0 / function(id) do_define_label(id, 0) end,  -- TEMP
    move = sp1 * t_identifier * (sp1 * t_define)^-2 / function(id) do_define_label(id, 0) end,  -- TEMP

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
-- (if there ever were any) and making our life harder else.
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


-- NOTE about prefixes: most is handled by all_alt_pattern(), however commands
-- that have no arguments and that are prefixes of other commands MUST be
-- suffixed with a "* #sp1" pattern.

local Ci = {
    -- these can appear anywhere in the script
    ["break"] = cmd(),
    ["return"] = cmd(),

    state = cmd(I),

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
    -- These three need more attention (different kind of labels; move
    -- additionally may accept 0 or 1):
    action = cmd(D),
    ai = cmd(D),
    move = sp1 * t_define * (sp1 * t_define)^0,

    cactor = cmd(D),
    count = cmd(D),
    cstator = cmd(D),
    cstat = cmd(D),
    clipdist = cmd(D),
    sizeto = cmd(D,D),
    sizeat = cmd(D,D),
    strength = cmd(D),
    addstrength = cmd(D),
    spritepal = cmd(D),

    hitradius = cmd(D,D,D,D,D),
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
    echo = cmd(R),
    starttrackvar = cmd(R),
    clearmapstate = cmd(R),
    activatecheat = cmd(R),
    setgamepalette = cmd(R),

    -- some commands taking defines
    addammo = cmd(D,D),  -- exec SPECIAL HANDLING!
    addweapon = cmd(D,D),  -- exec SPECIAL HANDLING!
    debris = cmd(D,D),
    addinventory = cmd(D,D),
    guts = cmd(D,D),

    -- cont'd
    addkills = cmd(D),
    addphealth = cmd(D),
    angoff = cmd(D),
    debug = cmd(D),
    endofgame = cmd(D),
    eqspawn = cmd(D),
    espawn = cmd(D),
    globalsound = cmd(D),
    lotsofglass = cmd(D),
    mail = cmd(D),
    money = cmd(D),
    paper = cmd(D),
    qspawn = cmd(D),
    quote = cmd(D),
    savenn = cmd(D),
    save = cmd(D),
    sleeptime = cmd(D),
    soundonce = cmd(D),
    sound = cmd(D),
    spawn = cmd(D),
    stopsound = cmd(D),

    eshoot = cmd(D),
    ezshoot = cmd(R,D),
    ezshootvar = cmd(R,R),
    shoot = cmd(D),
    zshoot = cmd(R,D),
    zshootvar = cmd(R,R),

    fall = cmd(),
    flash = cmd(),
    getlastpal = cmd(),
    insertspriteq = cmd(),
    killit = cmd(),  -- exec SPECIAL HANDLING!
    mikesnd = cmd(),
    nullop = cmd(),
    pkick = cmd(),
    pstomp = cmd(),
    resetactioncount = cmd(),
    resetcount = cmd(),
    resetplayer = cmd(),  -- exec SPECIAL HANDLING!
    respawnhitag = cmd(),
    tip = cmd(),
    tossweapon = cmd(),
    wackplayer = cmd(),

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
    palfrom = (sp1 * t_define)^-4,

    operate = cmd() * #sp1,

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

    redefinequote = sp1 * t_define * newline_term_string,
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
    -- XXX: ai, action, move/def labels
    ifai = cmd(D),
    ifaction = cmd(D),
    ifmove = cmd(D),

    ifrnd = cmd(D),
    ifpdistl = cmd(D),
    ifpdistg = cmd(D),
    ifwasweapon = cmd(D),
    ifactioncount = cmd(D),
    ifcount = cmd(D),
    ifactor = cmd(D),
    ifstrength = cmd(D),
    ifspawnedby = cmd(D),
    ifgapzl = cmd(D),
    iffloordistl = cmd(D),
    ifceilingdistl = cmd(D),
    ifphealthl = cmd(D),
    ifspritepal = cmd(D),
    ifgotweaponce = cmd(D),
    ifangdiffl = cmd(D),
    ifsound = cmd(D),
    ifpinventory = cmd(D,D),

    ifvarl = cmd(R,D),
    ifvarg = cmd(R,D),
    ifvare = cmd(R,D),
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

    ifp = (sp1 * t_define)^1,
    ifsquished = cmd(),
    ifserver = cmd(),
    ifrespawn = cmd(),
    ifoutside = cmd(),
    ifonwater = cmd(),
    ifnotmoving = cmd(),
    ifnosounds = cmd(),
    ifmultiplayer = cmd(),
    ifinwater = cmd(),
    ifinspace = cmd(),
    ifinouterspace = cmd(),
    ifhitweapon = cmd(),
    ifhitspace = cmd(),
    ifdead = cmd(),
    ifclient = cmd(),
    ifcanshoottarget = cmd(),
    ifcanseetarget = cmd(),
    ifcansee = cmd() * #sp1,
    ifbulletnear = cmd(),
    ifawayfromwall = cmd(),
    ifactornotstayput = cmd(),
}


----==== Tracing and reporting ====----
local string = require("string")

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
    local line = bsearch(g_newlineidxs, pos)
    assert(line and g_newlineidxs[line]<=pos and pos<g_newlineidxs[line+1])
    local col = pos-g_newlineidxs[line]
    return line+1, col-1
end

-- A generic trace function, prints a position together with the match content
-- A non-existing 'doit' means 'true'.
local function TraceFunc(pat, label, doit)
    pat = Pat(pat)

    if (doit==nil or doit) then
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
        if (not g_badids[a]) then
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
local function after_cmd_Cmt()
    if (g_numerrors == inf) then
--        print("Aborting parsing...")
        return nil  -- make the match fail, bail out of parsing
    end

    return true  -- don't return any captures
end

-- attach the command names at the front!
local function attachnames(kwtab)
    for cmdname,cmdpat in pairs(kwtab) do
        -- The match-time function capture at the end is so that every
        -- command acts as a barrier to captures to prevent stack overflow (and
        -- to make lpeg.match return a subject position at the end)
        kwtab[cmdname] = lpeg.Cmt(Keyw(cmdname) * cmdpat, after_cmd_Cmt)
    end
end

attachnames(Co)
attachnames(Ci)
attachnames(Cif)


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
local function warn_on_lonely_else()
    warnprintf("found `else' with no `if'")
end


local con_inner_command = all_alt_pattern(Ci)
local con_if_begs = all_alt_pattern(Cif)

local lone_else = ("else" * sp1)/warn_on_lonely_else

local stmt_list = Var("stmt_list")
-- possibly empty statement list:
local stmt_list_or_eps = (stmt_list * sp1)^-1
local stmt_list_nosp_or_eps = (stmt_list * (sp1 * stmt_list)^0)^-1

-- common to all three: <name/tilenum> [<strength> [<action> [<move> [<ai>... ]]]]
local common_actor_end = sp1 * t_define * sp1 * (t_define * sp1)^0 * stmt_list_or_eps * "enda"

--== block delimiters (no recursion) ==--
local Cb = {
    -- actor (...)
    actor = common_actor_end,
    -- eventloadactor (...)
    eventloadactor = common_actor_end,
    -- useractor <actortype> (...)
    useractor = sp1 * t_define * common_actor_end,

    onevent = sp1 * t_define * sp1 * stmt_list_or_eps * "endevent",

    state = sp1 * t_identifier * sp1 * stmt_list_or_eps * "ends",
}

attachnames(Cb)


local t_good_identifier = Range("AZ", "az", "__") * Range("AZ", "az", "__", "09")^0

-- CON isaltok also has chars in "{}.", but these could potentially
-- interfere with *CON* syntax.  The "]" is so that the number in array[80]
-- isn't considered a broken identifier.
-- "-" is somewhat problematic, but we allow it only as 2nd and up character, so
-- there's no ambiguity with unary minus.  (Commands must be separated by spaces
-- in CON, so a trailing "-" is "OK", too.)
-- This is broken in itself, so we ought to make a compatibility/modern CON switch.
local t_broken_identifier = BadIdent(-((t_number + t_good_identifier) * (sp1 + Set("[]:"))) *
                                     (alphanum + Set("_/\\*")) * (alphanum + Set("_/\\*-"))^0)

local function begin_if_fn()
    g_ifnestlevel = g_ifnestlevel+1
end

local function end_if_fn()
    g_ifnestlevel = g_ifnestlevel-1
end

local function check_else_Cmt()
    return (g_ifnestlevel==0)  -- match an 'else' only at the outermost level
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
    -- NOTE: -con.keyword alone would be wrong, e.g. "state breakobject":
    -- NOTE 2: The + "[" is so that stuff like
    --   getactor[THISACTOR].x x
    --   getactor[THISACTOR].y y
    -- is parsed correctly.  (Compared with this:)
    --   getactor[THISACTOR].x x
    --   getactor [THISACTOR].y y
    -- This is in need of cleanup!
    t_identifier = -NotKeyw(con.keyword * (sp1 + "[")) * lpeg.C(t_identifier_all),
    t_define = (t_maybe_minus * t_identifier/lookup_defined_label) + t_number,  -- TODO: minus

    t_arrayexp = t_identifier * arraypat * memberpat^-1,

    -- SWITCH
    switch_stmt = Keyw("switch") * sp1 * t_rvar *
        (Var("case_block") + Var("default_block"))^0 * sp1 * "endswitch",

    -- NOTE: some old DNWMD has "case: PIGCOP".  I don't think I'll allow that.
    case_block = (sp1 * Keyw("case") * sp1 * t_define * (sp0*":")^-1)^1 * sp1 *
        stmt_list_nosp_or_eps, -- * "break",

    default_block = sp1 * Keyw("default") * (sp0*":"*sp0 + sp1) * stmt_list_nosp_or_eps,  -- * "break",

    if_stmt = con_if_begs/begin_if_fn * sp1 * Var("single_stmt") * Pat("")/end_if_fn
        * (sp1 * lpeg.Cmt(Pat("else"), check_else_Cmt) * sp1 * Var("single_stmt"))^-1,

    -- TODO?: SST TC has "state ... else ends"
    while_stmt = Keyw("whilevarvarn") * sp1 * t_rvar * sp1 * t_rvar * sp1 * Var("single_stmt")
        + Keyw("whilevarn") * sp1 * t_rvar * sp1 * t_define * sp1 * Var("single_stmt"),

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


local math = require("math")

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


---=== EXPORTED FUNCTIONS ===---

function parse(contents)  -- local
    -- save outer state
    local lastkw, lastkwpos, numerrors = g_lastkw, g_lastkwpos, g_numerrors
    local newlineidxs = g_newlineidxs

    g_ifnestlevel = 0

    -- set up new state
    -- TODO: pack into one "parser state" table?
    g_lastkw, g_lastkwpos, g_numerrors = nil, nil, 0
    g_newlineidxs = setup_newlineidxs(contents)

    g_recurslevel = g_recurslevel+1

    local idx = lpeg.match(Grammar, contents)

    if (not idx) then
        printf("[%d] Match failed.", g_recurslevel)
        g_numerrors = inf
    elseif (idx == #contents+1) then
        if (g_numerrors ~= 0) then
            printf("[%d] Matched whole contents (%d errors).",
                   g_recurslevel, g_numerrors)
        elseif (g_recurslevel==0) then
            print("[0] Matched whole contents.")
        end
    else
        local i, col = getlinecol(idx)
        local bi, ei = g_newlineidxs[i-1]+1, g_newlineidxs[i]-1

        printf("[%d] Match succeeded up to %d (line %d, col %d; len=%d)",
               g_recurslevel, idx, i, col, #contents)
        g_numerrors = inf

--        printf("Line goes from %d to %d", bi, ei)
        local suffix = ""
        if (ei-bi > 76) then
            ei = bi+76
            suffix = " (...)"
        end
        print(string.sub(contents, bi, ei)..suffix)

        if (g_lastkwpos) then
            i, col = getlinecol(g_lastkwpos)
            printf("Last keyword was at line %d, col %d: %s", i, col, g_lastkw)
        end
    end

    g_recurslevel = g_recurslevel-1

    -- restore outer state
    g_lastkw, g_lastkwpos = lastkw, lastkwpos
    g_numerrors = (g_numerrors==inf and inf) or numerrors
    g_newlineidxs = newlineidxs
end


if (not _EDUKE32_LUNATIC) then
    --- stand-alone

    for argi=1,#arg do
        local filename = arg[argi]

        g_recurslevel = -1
        g_badids = {}
        reset_labels()
        reset_gamedata()

        g_numerrors = 0

        g_directory = filename:match("(.*/)") or ""
        filename = filename:sub(#g_directory+1, -1)

        local ok, msg = pcall(do_include_file, g_directory, filename)
        if (not ok) then
            if (g_lastkwpos ~= nil) then
                printf("LAST KEYWORD POSITION: %s, %s", linecolstr(g_lastkwpos), g_lastkw)
            end
            print(msg)
        end
    end
else
    --- embedded
    return { parse=parse }
end
