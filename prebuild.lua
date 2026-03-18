Workspace = {
  name = "mini-clinic-wks"
}

Project = {
  name = "mini-clinic",
  kind = "ConsoleApp",
  language = "C++",
  dialect = "20",

  pch = "include/pch.h",

  files = {
    "src/*.cpp"
  };

  includedirs = {
    "include",
  },

  links = {
    "curl",
  }
}
