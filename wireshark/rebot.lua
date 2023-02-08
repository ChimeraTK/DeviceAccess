-- SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
-- SPDX-License-Identifier: LGPL-3.0-or-later

-- Rebot dissector for Wireshark
rebot_protocol = Proto("Rebot", "Rebot Protocol")

-- Generic fields
command = ProtoField.int32("rebot.command", "command", base.DEC)

-- Payload fields

-- HELLO
magic = ProtoField.string("rebot.magic", "magic", base.ASCII)
version = ProtoField.int32("rebot.version", "version", base.DEC)

-- MULTI_WORD_READ
address = ProtoField.uint32("rebot.address", "address", base.HEX)
word_count = ProtoField.uint32("rebot.word_count", "word_count", base.HEX)

-- READ_ACK
data = ProtoField.uint32("rebot.data", "data", base.HEX)

rebot_protocol.fields = {
    -- Generic commands
    command,

    -- HELLO command
    magic, version,
    
    -- MULTI_WORD_READ, SINGLE_WORD_WRITE
    address,
    
    -- MULT_WORD_WRITE
    word_count,

    -- READ_ACK, SINGLE_WORD_WRITE
    data
}

function rebot_protocol.dissector(buffer, pinfo, tree)
    length = buffer:len()
    if length == 0 then return end

    pinfo.cols.protocol = rebot_protocol.name

    local subtree = tree:add(rebot_protocol, buffer(), "Rebot Protocol Data")
    local cmd = buffer(0,4):le_int()
    local cmd_name = get_command_name(cmd)
    subtree:add_le(command, buffer(0,4)):append_text(" (".. cmd_name .. ")")

    if cmd_name == "HELLO" then
        subtree:add_le(magic, buffer(4,4))

        local major = buffer(8,2):le_int()
        local minor = buffer(10,2):le_int()
        subtree:add_le(version, buffer(8,4)):append_text(" (" .. major .."."..minor..")")
    elseif cmd_name == "MULTI_WORD_READ" then
        subtree:add_le(address, buffer(4, 4))
        subtree:add_le(word_count, buffer(8,4))
    elseif cmd_name == "READ_ACK" then
        local offset = 4
        local remaining = length - 4
        while remaining > 0 do
            subtree:add_le(data, buffer(offset, 4))
            offset = offset + 4
            remaining = remaining - 4
        end
    elseif cmd_name == "SINGLE_WORD_WRITE" then
        subtree:add_le(address, buffer(4, 4))
        subtree:add_le(data, buffer(8,4))
    elseif cmd_name == "MULTI_WORD_WRITE" then
        subtree:add_le(address, buffer(4, 4))
        subtree:add_le(word_count, buffer(8,4))
        local word_count = buffer(8,4):le_int()
        local offset = 12
        while word_count > 0 do
            subtree:add_le(data, buffer(offset, 4))
            word_count = word_count - 1
            offset = offset + 4
        end
    elseif cmd_name == "WRITE_ACK" then
        -- nothing else happens here
    end
end

function get_command_name(cmd)
    local command_name = "Unknown"

    if cmd == 0 then command_name = "SINGLE_WORD_READ"
    -- Requests
    elseif cmd == 1 then command_name = "SINGLE_WORD_WRITE"
    elseif cmd == 2 then command_name = "MULTI_WORD_WRITE"
    elseif cmd == 3 then command_name = "MULTI_WORD_READ"
    elseif cmd == 5 then command_name = "PING"
    elseif cmd == 6 then command_name = "SET_SESSION_TIMEOUT"
    elseif cmd == 4 then command_name = "HELLO"

    -- Replys
    elseif cmd == 1000 then command_name = "READ_ACK"
    elseif cmd == 1001 then command_name = "WRITE_ACK"
    elseif cmd == 1005 then command_name = "PONG"
    elseif cmd == -1010 then command_name = "TOO_MUCH_DATA"
    elseif cmd == -1040 then command_name = "UNKNOWN_INSTRUCTION" end

    return command_name
end

local tcp_port = DissectorTable.get("tcp.port")
tcp_port:add(5001, rebot_protocol)