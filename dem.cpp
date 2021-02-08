#include "dem.h"
#include "parameters.h"
#include <iostream>
#include <sys/stat.h>

#ifdef HAS_GDAL
#include "gdal_priv.h"
#include "cpl_conv.h"
#endif

namespace Mpcv {

static bool fileExists(const std::string& name) {
    struct stat buffer;
	return (stat (name.c_str(), &buffer) == 0);
}

std::string getDsmFile(const std::string& file) {
	std::string base = file.substr(0, file.size() - 4);
	return base + "-dsm.tif";
}

#ifdef HAS_GDAL
TexturedMesh loadDem(std::string file, const Progress& progress) {
    std::string dsmFile = getDsmFile(file);
	std::string textureFile;
	bool textured = false;
	if (fileExists(dsmFile)) {
        std::cout << "Found DSM file " << dsmFile << std::endl;
		textureFile = file;
		file = dsmFile;
		textured = true;
	}

    static bool firstTime = true;
    if (firstTime) {
        GDALAllRegister();
        firstTime = false;
    }
    GDALDataset* dataset = (GDALDataset*)GDALOpen(file.c_str(), GA_ReadOnly);
    if (dataset == nullptr) {
        throw std::runtime_error("Cannot open GeoTIFF '" + file + "'");
    }

    printf("Driver: %s/%s\n",
           dataset->GetDriver()->GetDescription(),
           dataset->GetDriver()->GetMetadataItem(GDAL_DMD_LONGNAME));
    printf("Size is %dx%dx%d\n",
           dataset->GetRasterXSize(),
           dataset->GetRasterYSize(),
           dataset->GetRasterCount());
    if (dataset->GetProjectionRef() != nullptr) {
        printf("Projection is `%s'\n", dataset->GetProjectionRef());
    }
    double geoTransform[6];
    if (dataset->GetGeoTransform(geoTransform) == CE_None) {
        printf("Origin = (%.6f,%.6f)\n",
               geoTransform[0],
               geoTransform[3]);
        printf("Pixel Size = (%.6f,%.6f)\n",
               geoTransform[1],
               geoTransform[5]);
    }


    GDALRasterBand* rasterBand = dataset->GetRasterBand(1);
    int blockXSize, blockYSize;
    rasterBand->GetBlockSize(&blockXSize, &blockYSize);
    printf("Block=%dx%d Type=%s, ColorInterp=%s\n",
           blockXSize,
           blockYSize,
           GDALGetDataTypeName(rasterBand->GetRasterDataType()),
           GDALGetColorInterpretationName(
               rasterBand->GetColorInterpretation()));

    int bGotMin, bGotMax;
    double adfMinMax[2];
    adfMinMax[0] = rasterBand->GetMinimum(&bGotMin);
    adfMinMax[1] = rasterBand->GetMaximum(&bGotMax);
    if (!(bGotMin && bGotMax)) {
        GDALComputeRasterMinMax((GDALRasterBandH)rasterBand, TRUE, adfMinMax);
    }
    printf("Min=%.3fd, Max=%.3f\n", adfMinMax[0], adfMinMax[1]);
    if (rasterBand->GetOverviewCount() > 0) {
        printf("Band has %d overviews.\n", rasterBand->GetOverviewCount());
    }
    if (rasterBand->GetColorTable() != nullptr) {
        printf("Band has a color table with %d entries.\n",
               rasterBand->GetColorTable()->GetColorEntryCount());
    }

    uint32_t bandWidth = rasterBand->GetXSize();
    uint32_t bandHeight = rasterBand->GetYSize();
    std::cout << "Band width = " << bandWidth << std::endl;
    std::cout << "Band height = " << bandHeight << std::endl;

    double originX = geoTransform[0];
    double originY = geoTransform[3];
    double pixelX = geoTransform[1];
    double pixelY = geoTransform[5];
    std::cout << "Extents = " << originX << "," << originY + pixelY * bandHeight
              << ":" << originX + pixelX * bandWidth << "," << originY
              << std::endl;

    double nodata=  rasterBand->GetNoDataValue();
    std::cout << "No-data value = " << nodata << std::endl;
    TexturedMesh mesh;
    std::vector<float> scanline(bandWidth);
    Parameters& globals = Parameters::global();
    int step = std::max(std::max(bandWidth, bandHeight) / globals.dsmResolution, 1);
    uint32_t width = bandWidth / step;
    uint32_t height = bandHeight / step; 
    for (uint32_t y = 0; y <= height; ++y) {
        CPLErr err = rasterBand->RasterIO(GF_Read,
                                          0,
                                          std::min(y * step, bandHeight),
                                          bandWidth,
                                          1,
                                          scanline.data(),
                                          bandWidth,
                                          1,
                                          GDT_Float32,
                                          0,
                                          0);
        if (err != CPLE_None) {
            throw std::runtime_error("Error reading file '" + file + "'");
        }
        for (uint32_t x = 0; x <= width; ++x) {
            Pvl::Vec3f v(x * pixelX * step, y * pixelY * step, scanline[std::min(x * step, bandWidth - 1)]);
            mesh.vertices.push_back(v);
        }
		if (textured) {
	        for (uint32_t x = 0; x <= width; ++x) {
				Pvl::Vec2f v(float(x) / width, 1.f - float(y) / height);
				mesh.uv.push_back(v);
			}
		}
        if (progress(y * 100.f / height)) {
            return {};
        }
    }
    uint32_t index = 0;
    for (uint32_t y = 0; y <= height - 1; ++y, ++index) {
        for (uint32_t x = 0; x <= width - 1; ++x, ++index) {
            if (mesh.vertices[index][2] == nodata ||
                mesh.vertices[index + 1][2] == nodata ||
                mesh.vertices[index + width][2] == nodata ||
                mesh.vertices[index + width + 1][2] == nodata) {
                continue;
            }
            mesh.faces.emplace_back(TexturedMesh::Face{
                index + 1,
                index,
                index + width + 1,
            });
            mesh.faces.emplace_back(TexturedMesh::Face{
                index + 1,
                index + width + 1,
                index + width + 2,
            });

			if (textured) {
				mesh.texIds.push_back(mesh.faces[mesh.faces.size() - 2]);
				mesh.texIds.push_back(mesh.faces[mesh.faces.size() - 1]);
			}
        }
    }

    for (std::size_t vi = 0; vi < mesh.vertices.size(); ++vi) {
        if (mesh.vertices[vi][2] == nodata) {
            mesh.vertices[vi][2] = adfMinMax[0];
        }
    }

    if (textured) {
        mesh.texture = makeTexture(textureFile.c_str());
    }

    mesh.srs = Srs(Coords(originX, originY, 0.));
    return mesh;
}

#else
TexturedMesh loadDem(std::string, const Progress&) {
    throw std::runtime_error(
        "MPCV not linked with GDAL, please recompile with WITH_GDAL=ON");
}
#endif


} // namespace Mpcv
