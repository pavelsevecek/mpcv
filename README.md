# Mesh and Point Cloud Viewer

## Building
On Ubuntu system, the following packages are required:
- `cmake`
- `qt5-default`
- `libtbb-dev`

Additional optional dependencies are:
- `libjpeg-dev`  - needed for loading large jpeg textures (Qt can only handle up to 32767x32767)
- `libpng-dev`   - needed for loading large png textures
- `libgdal-dev`  - needed for loading GeoTIFFs as mesh

First, clone the repository and create a build directory using:
```
git clone --recursive https://github.com/pavelsevecek/mpcv.git
cd mpcv
mkdir build
cd build
```

Then, configure `mpcv` using `cmake` (use OFF if you did not install the optional libraries) and build it
```
cmake -DCMAKE_BUILD_TYPE=Release -DWITH_PNG=OFF -DWITH_JPEG=OFF -DWITH_GDAL=ON ..
make
```
