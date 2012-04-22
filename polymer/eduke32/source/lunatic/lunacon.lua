-- LunaCON CON to Lunatic translator
-- requires LPeg, http://www.inf.puc-rio.br/~roberto/lpeg/lpeg.html

local lpeg = require("lpeg")

local Pat, Set, Range, Var = lpeg.P, lpeg.S, lpeg.R, lpeg.V


---- All keywords pattern -- needed for CON syntax
local con_keyword = dofile("con_lang.lua")


local function match_until(matchsp, untilsp)  -- (!untilsp matchsp)* in PEG
    -- sp: string or pattern
    return (matchsp - Pat(untilsp))^0
end


----==== patterns ====----

---- basic ones
local newline = Pat("\n")  -- need to do stuff on newline later...
local anychar = Pat(1)
-- comments
local comment = "/*" * match_until(anychar, "*/") * "*/"
local linecomment = "//" * match_until(anychar, newline)
local whitespace = Var("whitespace")
local sp0 = whitespace^0
local sp1 = whitespace^1 -- + (-1)  -- matches EOF, too
local alpha = Range("AZ", "az")  -- locale?
local alphanum = alpha + Range("09")
local alnumtok = alphanum + Set("{}/\\*-_.")  -- see isaltok() in gamedef.c

--- basic lexical elements ("tokens")
local t_number = Range("09")^1 + (Pat("0x") + "0X")*Range("09", "af")^1
-- Valid identifier names are disjunct from keywords!
-- XXX: CON is more permissive with identifier name characters:
local t_identifier = Var("t_identifier")
-- This one matches keywords, too:
local t_identifier_all = Var("t_identifier_all")
local t_define = Var("t_define")
local t_filename = (anychar-Set(" \t\r\n"))^1 --alnumtok^1  -- XXX
local t_newline_term_str = match_until(anychar, newline)

-- new-style inline arrays and structures:
local t_arrayexp = Var("t_arrayexp")

-- defines and constants can take the place of vars that are only read:
local t_rvar = t_arrayexp + t_define
-- not so with written-to vars:
local t_wvar = t_arrayexp + t_identifier


---- helper patterns / pattern constructing functions
local maybe_quoted_filename = ('"' * t_filename * '"' + t_filename)
local newline_term_string = (whitespace-newline)^1 * t_newline_term_str  -- XXX: empty string?


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

    local spat = ""
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


-- The command names will be attached to the front of the patterns later!

--== Top level CON commands ==--
local Co = {
    --- 1. Preprocessor
    include = sp1 * maybe_quoted_filename,
    includedefault = cmd(),
    define = cmd(I,D),
    --define = sp1 * t_identifier * sp1 * t_define,

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
    definelevelname = n_defines(2) * newline_term_string,  -- XXX: TS
    defineskillname = sp1 * t_define * newline_term_string,
    definevolumename = sp1 * t_define * newline_term_string,

    definequote = sp1 * t_define * newline_term_string,
    defineprojectile = cmd(D,D,D),
    definesound = sp1 * t_define * sp1 * maybe_quoted_filename * n_defines(5),  -- XXX: TS

    -- XXX: need to see how that behaves with e.g. stuff like gamevar.ogg:
    music = match_until(sp1 * t_filename, con_keyword),

    --- 3. Game Settings
    -- gamestartup has 25/29 fixed defines, depending on 1.3D/1.5 version:
    gamestartup = (sp1 * t_define)^25 * (sp1 * t_define)^-4,
    spritenopal = cmd(D),
    spritenoshade = cmd(D),
    spritenvg = cmd(D),
    spriteshadow = cmd(D),

    spriteflags = cmd(D,D),  -- also see inner

    --- 4. Game Variables / Arrays
    gamevar = cmd(I,D,D),
    gamearray = cmd(I,D),

    --- 5. Top level commands that are also run-time commands
    action = sp1 * t_identifier * (sp1 * t_define)^-5,
    ai = sp1 * t_identifier * (sp1 * t_define)^0,
    move = sp1 * t_identifier * (sp1 * t_define)^-2,

    --- 6. Deprecated TLCs
    betaname = newline_term_string,
    enhanced = cmd(D),
}


--== Run time CON commands ==--
--- 1. Gamevar Operators
local varop = cmd(W,D)
local varvarop = cmd(W,R)

-- t_define is t_rvar without t_arrayexp, actually, so that no nesting is allowed:
local arraypat = sp0 * "[" * sp0 * t_define * sp0 * "]"
-- The member name must match keywords, too (_all), because e.g. cstat is a member
-- of sprite[]
-- XXX: actor and sprite members can have parm2, e.g. this is legal:
--   ifvare player[THISACTOR].gotweapon 4 0
-- but not parsed currently.
local memberpat = sp0 * "." * sp0 * t_identifier_all

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
    getuserdef = getstructcmd,
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
    setuserdef = setstructcmd,
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
    addammo = cmd(D,D),
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
    killit = cmd(),
    mikesnd = cmd(),
    nullop = cmd(),
    operate = cmd(),
    pkick = cmd(),
    pstomp = cmd(),
    resetactioncount = cmd(),
    resetcount = cmd(),
    resetplayer = cmd(),
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
    copy = sp1 * t_identifier * arraypat * sp1 * t_identifier * arraypat,
    setarray = sp1 * t_identifier * arraypat * sp1 * t_rvar,

    activatebysector = cmd(R,R),
    addlog = cmd(),
    addlogvar = cmd(R),
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
    digitalnumberz = cmd(W,R,R,R,R,R,R,R,R,R,R,R),  -- 1W 11R
    minitext = cmd(R,R,R,R,R),

    ldist = cmd(W,R,R),
    lineintersect = cmd(R,R,R,R,R,R,R,R,R,R,W,W,W,W),  -- 10R 4W
    rayintersect = cmd(R,R,R,R,R,R,R,R,R,R,W,W,W,W),  -- 10R 4W
    loadmapstate = cmd(),
    savemapstate = cmd(),
    movesprite = cmd(R,R,R,R,R,W),
    neartag = cmd(R,R,R,R,R,W,W,W,W,R,R),
    operateactivators = cmd(R),
    operatesectors = cmd(R),
    palfrom = (sp1 * t_define)^-4,

    myos = cmd(R,R,R,R,R),
    myospal = cmd(R,R,R,R,R,R),
    myospalx = cmd(R,R,R,R,R,R),
    myosx = cmd(R,R,R,R,R),

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

    ifp = (sp1 * t_define)^1,

    ifclient = cmd(),
    ifserver = cmd(),
    ifonwater = cmd(),
    ifinwater = cmd(),
    ifactornotstayput = cmd(),
    ifactorsound = cmd(),
    ifcansee = cmd(),
    ifhitweapon = cmd(),
    ifsquished = cmd(),
    ifdead = cmd(),
    ifcanshoottarget = cmd(),
    ifhitspace = cmd(),
    ifoutside = cmd(),
    ifmultiplayer = cmd(),
    ifinspace = cmd(),
    ifbulletnear = cmd(),
    ifrespawn = cmd(),
    ifinouterspace = cmd(),
    ifnotmoving = cmd(),
    ifawayfromwall = cmd(),
    ifcanseetarget = cmd(),
    ifnosounds = cmd(),

    ifvarl = cmd(R,D),
    ifvarg = cmd(R,D),
    ifvare = cmd(R,D),
    ifvarn = cmd(R,D),
    ifvarand = cmd(R,D),
    ifvaror = cmd(R,D),
    ifvarxor = cmd(R,D),
    ifvareither = cmd(R,D),

    ifactorsound = cmd(R,R),
    ifvarvarg = cmd(R,R),
    ifvarvarl = cmd(R,R),
    ifvarvare = cmd(R,R),
    ifvarvarn = cmd(R,R),
    ifvarvarand = cmd(R,R),
    ifvarvaror = cmd(R,R),
    ifvarvarxor = cmd(R,R),
    ifvarvareither = cmd(R,R),
}


----==== Tracing and reporting ====----
local string = require("string")

-- newlineidxs will contain the 1-based file offsets to "\n" characters
local newlineidxs = {}

local function printf(fmt, ...)
    print(string.format(fmt, ...))
end

-- Returns index into the sorted table tab such that
--   tab[index] <= searchelt < tab[index+1].
-- Preconditions:
--  tab[i] < tab[i+1] for 1 <= i < #tab
--  tab[1] <= searchelt < tab[#tab]
-- If tab has less than 2 elements, returns nil.
local function bsearch(tab, searchelt)
--    printf("bsearch(tab, %d)", searchelt)
    local l, r = 1, #tab
    local i

    if (r <= 1) then
        return
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

local function getlinecol(pos)
    local line = bsearch(newlineidxs, pos)
    local col = pos-newlineidxs[line]
    return line, col
end

-- A generic trace function, prints a position together with the match content
-- A non-existing 'doit' means 'true'.
local function TraceFunc(pat, label, doit)
    pat = Pat(pat)

    if (doit==nil or doit) then
        local function tfunc(subj, pos, a)
            local line, col = getlinecol(pos)
            printf("%d,%d:%s:%s", line, col, label, a)
            return true
        end
        pat = lpeg.Cmt(pat, tfunc)
    end
    return pat
end

-- These are tracers for specific patterns which can be disabled
-- if desired.
local function Keyw(kwname) return TraceFunc(kwname, "kw", true) end
local function NotKeyw(text) return TraceFunc(text, "!kw", true) end
local function Ident(idname) return TraceFunc(idname, "id", true) end
local function Stmt(cmdpat) return TraceFunc(cmdpat, "st", true) end


----==== Translator continued ====----
-- attach the command names at the front!
local function attachnames(kwtab)
    for cmdname,cmdpat in pairs(kwtab) do
        kwtab[cmdname] = Keyw(cmdname) * cmdpat
    end
end

attachnames(Co)
attachnames(Ci)
attachnames(Cif)


-- Takes one or more tables and +'s all its patterns together in the order of
-- appearance.
-- The tables must map command names to their patterns.
local function all_alt_pattern(...)
    local pat = Pat(false)
    local args = {...}
    for argi=1,#args do
        local pattab = args[argi]
        for cmdname,cmdpat in pairs(pattab) do
            pat = pat + cmdpat
        end
    end
    return pat
end

-- actor ORGANTIC is greeting!
local function warn_on_lonely_else(subj, pos)
    print(pos..": warning: found `else' with no `if'")
    return true
end

-- About prefixes: I think it's not a problem *here* if e.g. "getactor" comes
-- before "getactorvar", because the pattern for the former will fail
-- eventually in the ordered choice if fed with the latter.  However, it DOES
-- matter in the keyword list, see NotKeyw() trace function and comment in
-- con_lang.lua.
local con_outer_command = all_alt_pattern(Co)
local con_inner_command = all_alt_pattern(Ci)
local con_if_begs = all_alt_pattern(Cif)

local lone_else = lpeg.Cmt("else" * sp1, warn_on_lonely_else)

local stmt_list = Var("stmt_list")
-- possibly empty statement list:
local stmt_list_or_eps = (stmt_list * sp1)^-1

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


--- The final grammar!
local Grammar = Pat{
    -- The starting symbol.
    -- A translation unit is a (possibly empty) sequence of outer CON
    -- commands, separated by at least one whitespace which may be
    -- omitted at the EOF.
    sp0 * ((con_outer_command + all_alt_pattern(Cb)) * (sp1 + (-1)))^0,

    -- Deps.  These appear here because we're hitting a limit with LPeg else:
    -- http://lua-users.org/lists/lua-l/2008-11/msg00462.html
    whitespace = Set(" \t\r") + newline + Set("(),;") + comment + linecomment,

    t_identifier_all = Range("AZ", "az", "__") * Range("AZ", "az", "__", "09")^0,
    -- NOTE: -con_keyword alone would be wrong, e.g. "state breakobject":
    -- NOTE 2: The + "[" is so that stuff like
    --   getactor[THISACTOR].x x
    --   getactor[THISACTOR].y y
    -- is parsed correctly.  (Compared with this:)
    --   getactor[THISACTOR].x x
    --   getactor [THISACTOR].y y
    -- This is in need of cleanup!
    t_identifier = -NotKeyw(con_keyword * (sp1 + "[")) * Ident(t_identifier_all),
    t_define = Pat("-")^-1 * sp0 * (t_identifier + t_number),

    t_arrayexp = t_identifier * arraypat * memberpat^-1,

    switch_stmt = Keyw("switch") * (sp1 * (Var("case") + Var("default")))^0 * sp1 * "endswitch",
    case = Keyw("case") * sp1 * t_define * sp0 * Pat(":")^-1 * sp1 * stmt_list_or_eps * "break",
    default = Keyw("default") * sp0 * Pat(":")^-1 * sp1 * stmt_list_or_eps * "break",

    -- The "lone" if statement is tested first, so that a potential dangling "else" is
    -- attached to the outermost possible "if", as done by CON
    if_stmt = con_if_begs * sp1 * Var("single_stmt") * -(sp1 * Pat("else"))
        + con_if_begs * sp1 * Var("single_stmt") * sp1 * "else" * sp1 * Var("single_stmt"),

    while_stmt = Keyw("whilevarvarn") * sp1 * t_rvar * sp1 * t_rvar * sp1 * Var("single_stmt")
        + Keyw("whilevarn") * sp1 * t_rvar * sp1 * t_define * sp1 * Var("single_stmt"),

    -- TODO: some sp1 --> sp0?
    single_stmt = Stmt(
        lone_else^-1 *
            ( Keyw("{") * sp0 * "}"
              + Keyw("{") * sp1 * stmt_list * sp1 * "}"
              + (con_inner_command + Var("switch_stmt") + Var("if_stmt") + Var("while_stmt"))
--              + lpeg.Cmt(t_newline_term_str, function (subj, curpos) print("Error at "..curpos) end)
            )),

    -- a non-empty statement/command list
    stmt_list = Var("single_stmt") * (sp1 * Var("single_stmt"))^0,
}


local math = require("math")

local function setup_newlineidxs(contents)
    newlineidxs = {}
    for i in string.gmatch(contents, "()\n") do
        newlineidxs[#newlineidxs+1] = i
    end
    newlineidxs[#newlineidxs+1] = #contents+1  -- dummy newline
end

---=== stand-alone: ===---
if (not EDUKE32_LUNATIC) then
    local io = require("io")

    local filename = arg[1]
    assert(filename)

    local contents = io.open(filename):read("*all")
    setup_newlineidxs(contents)

    local idx = lpeg.match(Grammar, contents)

    if (not idx) then
        print("Match failed.")
        return
    end

    if (idx == #contents+1) then
        print("Matched whole contents.")
        return
    end

    local i, col = getlinecol(idx)
    local bi, ei = newlineidxs[i]+1, newlineidxs[i+1]-1

    printf("Match succeeded up to %d (line %d, col %d; len=%d)",
           idx, i, col, #contents)

--    printf("Line goes from %d to %d", bi, ei)
    print(string.sub(contents, bi, ei))
end
