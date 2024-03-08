# Fast Fourier Transform in WebGPU!

Simple implementation of the fast fourier transform algorithm, using c++ compiled to webassembly and run in the browser using webgpu.

## How to run

> **Note**: You need to have `emscripten` installed in your machine to compile the c++ code to webassembly. In the cmake file, change the path to your `emscripten` installation.

```bash
mkdir build

cd build

emcmake cmake ..

make
```

After that, you can run a local server to see the result in the browser. There is a simple `express` server in the `server` folder.

```bash
npm install

node index.js
```
