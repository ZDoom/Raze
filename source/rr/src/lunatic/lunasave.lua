
-- Savegame facilities for Lunatic.

local string = require("string")
local table = require("table")

local format = string.format

local assert = assert
local getmetatable = getmetatable
local pairs = pairs
local setmetatable = setmetatable
local tostring = tostring
local type = type

local cansave_cdata = require("lprivate").cansave_cdata


module(...)


---=== Serialization, based on the idea from PiL ===---

assert(tostring(0/0)=="nan")
assert(tostring(1/0)=="inf")

-- Serialize a 'primitive' Lua value.
local function basicSerialize(o)
    -- Compare with sb_get_initial_strbuf() below.
    -- XXX: nil?
    if (type(o) == "number") then
        -- NOTE: NaN and infinity handled.
        return tostring(o)
    elseif (type(o) == "boolean") then
        return o and "t" or "f"
    elseif (type(o) == "string") then
        -- TODO: return refname if it's shorter
        return format("%q", o)
    end
end


local function isSerializeable(obj)
    return (getmetatable(obj)=="serializeable" or cansave_cdata(obj))
end

-- 'Save buffer' class.  Objects of this class keep track of a string buffer
-- contiaining Lua code to recreate the values of particular variables from a
-- user Lunatic environment.
local savebuffer_mt = {
    __index = {
        -- Called on starting processing one module.
        startmod = function(self, modname)
            self:addrawf("if (N==%q) then", modname)
            -- Will contain all tables and cdata refs of this module, indexed
            -- by number.
            self:addraw("local T={}")
            self:resetRefs()
        end,

        -- Called on finishing processing one module.
        endmod = function(self)
            self:addraw("end")
        end,

        --== vvv INTERNAL vvv ===---
        resetRefs = function(self)
            self.numrefs = 0
            self.recursionDepth = 0
            self.val2ref = {}
            self.havereq = {}
        end,

        -- Get the code necessary to create this object, usually 'require'ing a
        -- module into a local variable.
        getRequire = function(self, value)
            local reqcode = value:_get_require()
            if (self.havereq[reqcode] == nil) then
                self.havereq[reqcode] = true
                self.strbuf[#self.strbuf+1] = reqcode
            end
        end,

        emitT = function(self, refcode, valcode, obj)
            self:addrawf("%s=%s", refcode, valcode)
            self.val2ref[obj] = refcode
        end,

        emitAssign = function(self, lhscode, refcode)
            self:addrawf("%s=%s", lhscode, refcode)
        end,
        --== ^^^ INTERNAL ^^^ ===---

        -- Add an entry of Lua object <value> that can be referenced by
        -- <lhscode> on the left hand side of an assignment.
        -- <excludedKeys>: if non-nil, should be table
        --  [<key_string>] = true. See EXCLUDE_KEYS below.
        -- Returns 'true' if <value> cannot be serialized.
        add = function(self, lhscode, value, excludedKeys)
            local valcode = basicSerialize(value)

            if (valcode == nil) then
                -- <value> is a not a 'basic' Lua object, but one passed by
                -- reference.
                if (not self.val2ref[value]) then
                    -- Object is being serialized for the first time.

                    -- Create a slot in 'T' for this object by which we can
                    -- refer to it in the following.
                    self.numrefs = self.numrefs+1
                    local refcode = format("T[%d]", self.numrefs)

                    if (isSerializeable(value)) then
                        -- We have a serializeable object from Lunatic
                        -- (e.g. actorvar).
                        self:getRequire(value)
                        local restoreCode = value:_serialize()
                        if (restoreCode == nil) then  -- XXX: check UNUSED?
                            return true
                        end
                        self:emitT(refcode, restoreCode, value)
                        valcode = refcode

                    elseif (type(value)=="table") then
                        -- Create a new table for this gamevar.
                        -- TODO: emit table initializations where possible.
                        self:emitT(refcode, "{}", value)

                        for k,v in pairs(value) do
                            local keystr = basicSerialize(k)
                            if (keystr == nil) then
                                return true
                            end

                            -- EXCLUDE_KEYS
                            local isNORESET = (self.recursionDepth == 0 and
                                                   excludedKeys and excludedKeys[k])

                            -- Generate the name under which the table element
                            -- is referenced.
                            local refcode2 = format("%s[%s]", refcode, keystr)

                            if (isNORESET) then
                                self:emitAssign(refcode2, format("M._V[%s]", keystr))
                            else
                                -- Recurse!
                                self.recursionDepth = self.recursionDepth+1
                                local ret = self:add(refcode2, v)
                                self.recursionDepth = self.recursionDepth-1

                                if (ret) then
                                    return true
                                end
                            end
                        end

                        valcode = refcode
                    else
                        -- We have anything else: can't serialize.
                        return true
                    end
                else
                    -- Object was previously serialized, get Lua expression it
                    -- can be referenced with.
                    valcode = self.val2ref[value]
                end
            end

            self:emitAssign(lhscode, valcode)
        end,

        -- Add a single string to the buffer.
        addraw = function(self, str)
            self.strbuf[#self.strbuf+1] = str
        end,

        -- Add a single formatted string to the buffer.
        addrawf = function(self, fmt, ...)
            self:addraw(format(fmt, ...))
        end,

        -- Get the Lua code recreating the values as a string.
        getcode = function(self)
            self.strbuf[#self.strbuf+1] = ""  -- add newline at EOS
            return table.concat(self.strbuf, "\n")
        end,
    }
}

local function sb_get_initial_strbuf()
    return {
        "local nan, inf = 0/0, 1/0",
        "local t, f = true, false",
    }
end

-- Create a new savebuffer object.
function savebuffer()
    -- .numrefs: how many table or cdata objects we have serialized
    -- .val2ref: [<Lua object>] = <Lua code string>
    -- .havereq = [<string>] = true
    -- .strbuf: array of Lua code pieces
    local sb = {
        numrefs=0, val2ref={}, havereq={},
        recursionDepth = 0,
        strbuf=sb_get_initial_strbuf()
    }

    return setmetatable(sb, savebuffer_mt)
end
