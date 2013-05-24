
-- Savegame facilities for Lunatic.

local string = require("string")
local table = require("table")

local getmetatable = getmetatable
local pairs = pairs
local setmetatable = setmetatable
local tostring = tostring
local type = type


module(...)


---=== Serialization, based on the idea from PiL ===---

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
        return string.format("%q", o)
    end
end


local function isSerializeable(obj)
    return (getmetatable(obj)=="serializeable")
end

-- 'Save buffer' class.  Objects of this class keep track of a string buffer
-- contiaining Lua code to recreate the values of particular variables from a
-- user Lunatic environment.
local savebuffer_mt = {
    __index = {
        -- Add an entry of Lua object <value> that can be referenced by
        -- <refcode> (which should be Lua code that can appear on both sides of
        -- an assignment).
        -- Returns 'true' if <value> cannot be serialized.
        add = function(self, refcode, value)
            local valcode = basicSerialize(value)
            local havetab = false

            if (valcode == nil) then
                -- <value> is a not a 'basic' Lua object, but one passed by
                -- reference.
                if (not self.val2ref[value]) then
                    -- Object is being serialized for the first time.

                    if (isSerializeable(value)) then
                        -- We have a serializeable object from Lunatic
                        -- (e.g. actorvar).
                        -- TODO: clean up (e.g. clear default values for actorvar).

                        -- First, get the code necessary to create this object,
                        -- usually 'require'ing a module into a local variable.
                        local reqcode = value:_get_require()
                        if (self.havereq[reqcode] == nil) then
                            self.havereq[reqcode] = true
                            self.strbuf[#self.strbuf+1] = reqcode
                        end

                        valcode = value:_serialize()

                    elseif (type(value)=="table") then
                        -- We have a Lua table.
                        havetab = true

                        -- Clear the table instead of creating a new one.
                        -- This way, local references to it don't become stale.
                        self:addrawf("ct(%s)", refcode)

                        for k,v in pairs(value) do
                            local keystr = basicSerialize(k)
                            if (keystr == nil) then
                                return true
                            end

                            if (type(v)=="table" and not isSerializeable(v)) then
                                -- nested tables: NYI
                                return true
                            end

                            -- Generate the name under which the table element
                            -- is referenced.
                            local refcode2 = string.format("%s[%s]", refcode, keystr)

                            -- Recurse!
                            self:add(refcode2, v)
                        end
                    else
                        -- We have anything else: can't serialize.
                        return true
                    end

                    self.val2ref[value] = refcode
                else
                    valcode = self.val2ref[value]
                end
            end

            if (not havetab) then
                self:addraw(refcode.."="..valcode)
            end
        end,

        -- Add a single string to the buffer.
        addraw = function(self, str)
            self.strbuf[#self.strbuf+1] = str
        end,

        -- Add a single formatted string to the buffer.
        addrawf = function(self, fmt, ...)
            self:addraw(string.format(fmt, ...))
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
        "local pairs = assert(pairs)",
        -- Clear table function:
        "local function ct(t)",
        "  for k in pairs(t) do t[k]=nil end",
        "end",
    }
end

-- Create a new savebuffer object.
function savebuffer()
    -- .val2ref: [<Lua object>] = <Lua code string>
    -- .havereq = [<string>] = true
    -- .strbuf: array of Lua code pieces
    local sb = { val2ref={}, havereq={}, strbuf=sb_get_initial_strbuf() }
    return setmetatable(sb, savebuffer_mt)
end
