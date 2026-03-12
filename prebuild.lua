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
    "vendor/llama/include"
  },

  links = {
    "llama"
  }
}

External = "vendor/llama"
