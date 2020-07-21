#include "e57.h"
#include "E57SimpleReader.h"

namespace Mpcv {

TexturedMesh loadE57(std::string file, const Progress& prog) {
    e57::Reader reader(file);
    int scanIndex = 0;
    e57::Data3D scanHeader;
    reader.ReadData3D(scanIndex, scanHeader);

    int64_t nColumn = 0;
    int64_t nRow = 0;
    int64_t nPointsSize = 0;
    int64_t nGroupsSize = 0;
    int64_t nCountsSize = 0;
    bool bColumnIndex;
    reader.GetData3DSizes(scanIndex, nRow, nColumn, nPointsSize, nGroupsSize, nCountsSize, bColumnIndex);
    int64_t nSize = (nRow > 0) ? nRow : 1024;

    float* xData = new float[nSize];
    float* yData = new float[nSize];
    float* zData = new float[nSize];
    uint8_t* rData = new uint8_t[nSize];
    uint8_t* gData = new uint8_t[nSize];
    uint8_t* bData = new uint8_t[nSize];
    e57::Data3DPointsData data;
    data.cartesianX = xData;
    data.cartesianY = yData;
    data.cartesianZ = zData;
    data.colorRed = rData;
    data.colorGreen = gData;
    data.colorBlue = bData;

    e57::CompressedVectorReader dataReader = reader.SetUpData3DPointsData(scanIndex, nSize, data);

    TexturedMesh mesh;
    unsigned long size = 0;
    while ((size = dataReader.read()) > 0) {
        for (unsigned long i = 0; i < size; i++) {
            mesh.vertices.push_back(Pvl::Vec3f(xData[i], yData[i], zData[i]));
            mesh.colors.push_back(Color(rData[i], gData[i], bData[i]));
        }
    }

    dataReader.close();
    delete xData;
    delete yData;
    delete zData;
    delete rData;
    delete gData;
    delete bData;
    return mesh;
}

} // namespace Mpcv
