Volume Indirect Illumination using Layered Polygonal Area Lights
===

<a rel="license" href="http://creativecommons.org/licenses/by-nc-sa/4.0/"><img alt="Creative Commons License" style="border-width:0" src="https://i.creativecommons.org/l/by-nc-sa/4.0/88x31.png" /></a><br />

This is an official implementation of the paper "[Real-time Indirect Illumination of Heterogeneous Emissive Volumes using Layered Polygonal Area Lights](http://tatsy.github.io/projects/kuge2019lpal/)". For more details, please visit our [project page](http://tatsy.github.io/projects/kuge2019lpal/)!

# Build & Run

Use [CMake](https://cmake.org/) to build the program. Our program depends also on [https://www.glfw.org/](GLFW) and [https://glm.g-truc.net/0.9.9/index.html](GLM). Please install these libraries to your computer beforehand.

For **Windows**, you can build it ordinarily using CMake GUI and Visual Studio.

For **Linux**, you can build and run the program by the following commands.

```sh
>> # Build
>> git clone https://github.com/Paul8029/IndirectIlluminationLPAL.git
>> cd IndirectIlluminationLPAL
>> mkdir build && cd build
>> cmake -DGLFW3_DIR=$PATH_TO_GLFW3 -DGLM_DIR=$PATH_TO_GLM -DCMAKE_BUILD_TYPE=Release ..
>> make -j4
>> # Now, you can find "main" executable in "build/bin".
>> cd bin
>> ./main ../../data/config.txt  # Please specify your own config if necessary
```

We tested the program using `Visual Studio 2019` and `GNU GCC v8.3.0 (g++)` with `C++17 filesystem`, so please make sure your compiler supports `filesystem` when using another complier. Also, our program uses `compute shader` of GLSL and tested using OpenGL v4.5.

When running the program, please specify [`config.txt`](./data/config.txt) to the executable. If you wish to test your own volume data, please modify `volumeFolder` section in it.

# Example

We included a simple static volume in `data/density.vol` and `data/emission.vol`. Also, you can download our animated data from [Google Drive](https://drive.google.com/open?id=1bskkeUxMsGvl82AKkWlabDu0lfCygz-_).

# Demo movie

[![](https://img.youtube.com/vi/9WfmoZGQ5fc/0.jpg)](https://www.youtube.com/watch?v=9WfmoZGQ5fc)

# License

This work is licensed under a <a rel="license" href="http://creativecommons.org/licenses/by-nc-sa/4.0/">Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License</a>.

(c) Takahiro Kuge, Tatsuya Yatagawa and Shigeo Morishima. (If you are interested in the commercial use, please contact us)
