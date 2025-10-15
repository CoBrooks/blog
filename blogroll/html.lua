local function html_to_string(node, indent)
  indent = (" "):rep(indent or 0)

  local str = ""

  if node.tag == "html" then
    str = "<!doctype html>\n"
  end

  str = str .. indent .. "<" .. node.tag

  for k, v in pairs(node.inner) do
    if type(k) == "number" then goto continue end

    str = str .. " " .. k .. "="

    if type(v) == "string" then
      str = str .. "\"" .. v .. "\""
    elseif type(v) == "number" or type(v) == "boolean" then
      str = str .. v
    else
      error("Invalid attribute key: " .. k .. " value: " .. v)
    end

    ::continue::
  end

  if node.void then
    return str .. " />"
  end

  str = str .. ">"

  for _, v in ipairs(node.inner) do
    local content = v

    if type(content) == "function" then
      content = v()
    end

    if content.tag then
      str = str .. "\n" .. html_to_string(content, #indent + 2)
    else
      str = str .. "\n  " .. indent .. tostring(content)
    end
  end

  str = str .. "\n" .. indent .. "</" .. node.tag .. ">"

  return str
end

local html = setmetatable({}, {
  __index = function(_, key)
    local void_elems = {
      ["meta"] = true,
    }

    return function(inner)
      return setmetatable({ tag = key, inner = inner, void = void_elems[key] or false }, {
        __tostring = html_to_string
      })
    end
  end,
})

return html
