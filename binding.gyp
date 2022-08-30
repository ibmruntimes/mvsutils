{
    "targets": [{
        "target_name": "mvsutils",
        "include_dirs": [
            "<!@(node -p \"require('node-addon-api').include\")"
        ],
        "conditions": [
          [ "OS==\"zos\"", {
            "sources": [
               "addon.cc",
               "filescan.cc",
               "errstring.cc",
               "console.cc"
            ],
          }],
        ],

        "libraries": [],
        "dependencies": [
            "<!(node -p \"require('node-addon-api').gyp\")"
        ],
        "defines": [ "NAPI_DISABLE_CPP_EXCEPTIONS" ]
    }]
}
