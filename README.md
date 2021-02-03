# Mesh and Point Cloud Viewer

## Building
On Ubuntu system, the following packages are required:
- `cmake`
- `qt5-default`
- `libtbb-dev`
- `libxerces-c-dev`

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

## UI controls
This help is also available `help -> controls`.
- Ctrl+[1-9] -  view only n-th mesh
- Shift+[1-9] - toggle visibility of n-th mesh
- Ctrl+I -      invert visibility
- Ctrl+A -      show all meshes
- Ctrl+Left -   show the mesh above the current
- Ctrl+Right -  show the mesh below the current
- Ctrl+Click -  show only the selected mesh
- Ctrl+Mouse wheel -   change the field of view
- Alt+Mouse wheel -    change the size of points
- Shift+Mouse wheel -  change the point stride
- Double click -       center the camera at target
