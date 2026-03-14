Workspace = {
  name = "mini-clinic-wks"
}

Project = {
  name = "mini-clinic",
  kind = "ConsoleApp",
  language = "C++",
  dialect = "20",

  files = {
    "src/*.cpp"
  };

  includedirs = {
    "include",
    "vendor/yaml-cpp/include"
  },

  links = {
    "curl",
    "yaml-cpp",
  }
}

External = "vendor/yaml-cpp"
