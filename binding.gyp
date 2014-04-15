{
  "variables": {
    "buffer_impl" : "<!(node -pe 'v=process.versions.node.split(\".\");v[0] > 0 || v[0] == 0 && v[1] >= 11 ? \"POS_0_11\" : \"PRE_0_11\"')",
    "callback_style" : "<!(node -pe 'v=process.versions.v8.split(\".\");v[0] > 3 || v[0] == 3 && v[1] >= 20 ? \"POS_3_20\" : \"PRE_3_20\"')"
  },
  "targets": [
    {
      "target_name": "openvg",
      "sources": [
        "src/openvg.cc",
        "src/egl.cc"
      ],
      "defines": [
        "NODE_BUFFER_TYPE_<(buffer_impl)",
        "TYPED_ARRAY_TYPE_<(buffer_impl)",
        "V8_CALLBACK_STYLE_<(callback_style)"
      ],
      "ldflags": [
        "-lGLESv2 -lEGL -lOpenVG -lSDL2",
      ],
      "cflags": [
        "-DENABLE_GDB_JIT_INTERFACE",
        "-Wall",
        "-I/usr/include/SDL2 -D_REENTRANT"
      ],
    },
    {
      "target_name": "init-egl",
      "sources": [
        "src/init-egl.cc"
      ],
      "ldflags": [
        "-lGLESv2 -lEGL -lOpenVG -lSDL2",
      ],
      "cflags": [
        "-DENABLE_GDB_JIT_INTERFACE",
        "-Wall",
        "-I/usr/include/SDL2 -D_REENTRANT"
      ],
    },
  ]
}
