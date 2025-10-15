local html = require("html")

local entries = {}

local f, err = loadfile("out.lua", "t", {
  entry = function(details)
    table.insert(entries, html.li {
      html.h3 { html.a { href = details.link, details.title } },
      html.time { datetime = details.date, details.date },
      html.address { rel = "author", details.source },
    })
  end,
})

assert(f, err)

f()

local roll = html.html {
  html.head {
    html.meta { name = "viewport", content = "width=device-width, initial-scale=1" },
    html.title { "Blogroll!" },
    html.style { rel = "text/css", [[
li {
  list-style: none;
}
li > h3 {
  margin-bottom: 0;
}
main {
  margin-inline: auto;
  max-width: min(70ch, 100% - 4rem);
}
:root {
  color-scheme: light dark;
}
a {
  text-decoration: none;
}
body {
  font-family: "Segoe UI", system-ui;
  font-size: 1.25rem;
  line-height: 1.5;
}
ul {
  padding: 0;
}
]] },
  },

  html.body {
    html.ul(entries)
  },
}

print(roll)
