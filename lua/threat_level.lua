-- ========================================================================
-- Licensed to Qualys, Inc. (QUALYS) under one or more
-- contributor license agreements.  See the NOTICE file distributed with
-- this work for additional information regarding copyright ownership.
-- QUALYS licenses this file to You under the Apache License, Version 2.0
-- (the "License"); you may not use this file except in compliance with
-- the License.  You may obtain a copy of the License at
--
-- http://www.apache.org/licenses/LICENSE-2.0
--
-- Unless required by applicable law or agreed to in writing, software
-- distributed under the License is distributed on an "AS IS" BASIS,
-- WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
-- See the License for the specific language governing permissions and
-- limitations under the License.
-- ========================================================================

local ibmod = ...

-- ========================================================================
-- This module tracks events and generates a threat level based on these
-- events.  This threat level is exported as a numeric THREAT_LEVEL field,
-- which can be used by other rules/modules.
--
-- Currently, the threat level is just the average severity of all events
-- that are not suppressed and have a confidence above a configured
-- minimum value.
--
-- Directives:
--
--  ThreatLevelMinConfidence <num>
--
--    Defines the minimum acceptable event confidence included in the
--    threat level calculation.
-- ========================================================================


-- ---------------------------------------------------------
-- Handle "ThreatLevelMinConfidence <num>" directive
-- ---------------------------------------------------------
local rc = ibmod:register_param1_directive(
    "ThreatLevelMinConfidence",
    function(ib_module, module_config, name, param1)
        local numval = tonumber(param1)

        -- Validate parameter
        if ((numval < 0) or (numval > 100)) then
            ibmod:logError("Directive \"%s %s\" value must be in range 0-100",
                            name, param1)
            return nil
        end

        -- Store in the config
        module_config["min_confidence"] = numval

        return 0
    end
)

-- ---------------------------------------------------------
-- Adjust the THREAT_LEVEL field, based on current events.
--
-- NOTE: The audit log will look for a numeric
--       THREAT_LEVEL field and log this value
--       as the threat level if it exists.
-- ---------------------------------------------------------
local adjust_threat_level = function(ib)
    ib:logInfo("Handling event=%s", ib.event_name)

    local num_events = 0
    local threat_level = 0
    local min_confidence = ib.config["min_confidence"]

    -- TODO: Where can we set the default?
    if min_confidence == nil then
        min_confidence = 0
    end

    --[[ Run through events, calculating the average
         severity as the "threat level".  Only include
         events that are not suppressed and have a
         confidence that is above the minimum. ]]
    for i,evt in ib:events() do
        ib:logInfo("EVENT: suppress=%s, confidence=%d, severity=%d",
                   evt:getSuppress(), evt:getConfidence(), evt:getSeverity())

        if evt:getSuppress() == "none" then
            local c = evt:getConfidence()

            if c >= min_confidence then
                num_events = num_events + 1
                threat_level = threat_level + evt:getSeverity()
            else
                ib:logInfo("Ignoring low confidence event: %s", evt:getMsg())
            end
        else
            ib:logInfo("Ignoring suppressed event: %s", evt:getMsg())
        end
    end
    if num_events > 0 then
        threat_level = math.floor(threat_level / num_events)
    end

    ib:logInfo("Adjusted THREAT_LEVEL: %d", threat_level)
    ib:set("THREAT_LEVEL", threat_level)

    return 0
end

-- ---------------------------------------------------------
-- Adjust the threat level at various places in the
-- transaction cycle.
--
-- TODO: Need to be able to be notified when an event
--       is added/updated/deleted
-- ---------------------------------------------------------
ibmod:handle_request_header_event(adjust_threat_level)
ibmod:handle_request_event(adjust_threat_level)
ibmod:handle_response_header_event(adjust_threat_level)
ibmod:handle_response_event(adjust_threat_level)

-- Return IB_OK.
return 0
