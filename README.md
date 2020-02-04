# PBR lab

`pbrlab` is well-verified(through brute force human verification and debugging) path tracing + PBR shading/rendering implementation.

`pbrlab` is good for your verify your renderer and PBR shading.

## Features

* PrincipledBSDF
  * with random-walk(brute force) pathtraced subsurface scattering. 
    * https://disney-animation.s3.amazonaws.com/uploads/production/publication_asset/153/asset/siggraph2016SSS.pdf https://disney-animation.s3.amazonaws.com/uploads/production/publication_asset/153/asset/siggraph2016SSS.pdf
    * Extending the Disney BRDF to a BSDF with Integrated Subsurface Scattering https://blog.selfshadow.com/publications/s2015-shading-course/
* Cycles and Arnold compatible shading parameters.

## Requirements

* OpenGL 3.x
* cmake
* C++11 or later compiler

## Supported platform

* [x] Linux
  * [x] x64_64
  * [ ] aarch64
* [ ] Windows 10 64bit
* [ ] macOS

## Building

```
$ ./scripts/bootstrap-linux.sh
$ cd build
$ make
```

## License

pbrlab is licensed under MIT license.

### Third party license

* TinyObjLoader : MIT license.
* TinyEXR : BSD license.
* imgui : MIT license.
* stb_image, stb_image_write, stb_image_resize : Public domain
* glfw3 : zlib/libpng license.
* glad : unlicense?
* ionicons : MIT license.
* Roboto font : Apache 2.0 license.
* toml11 : MIT license.
* rapidjson : MIT license.
* StaticJSON : MIT license.
* ghc filesystem : BSD-3 license.
* mpark variant : BSD-1 license.
* Embree-aarch64 : Apache-2.0
* pcg-random : Apache-2.0
