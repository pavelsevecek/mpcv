#include "e57.h"
#include "E57SimpleReader.h"
#include <iostream>

namespace Mpcv {

TexturedMesh loadE57(std::string file, const Progress& prog) {
    e57::Reader reader(file);
    int scanIndex = 0;
    e57::Data3D scanHeader;
    reader.ReadData3D(scanIndex, scanHeader);
    const auto& tr = scanHeader.pose.translation;
    std::cout << "Translation = " << tr.x << "," << tr.y << "," << tr.z << std::endl;

    int64_t column = 0;
    int64_t row = 0;
    int64_t pointsSize = 0;
    int64_t groupsSize = 0;
    int64_t countsSize = 0;
    bool columnIndex;
    reader.GetData3DSizes(scanIndex, row, column, pointsSize, groupsSize, countsSize, columnIndex);
    std::cout << "Loading E57 with " << pointsSize << " points" << std::endl;
    int64_t rowSize = (row > 0) ? row : 1024;

    std::vector<float> x(rowSize), y(rowSize), z(rowSize);
    std::vector<uint8_t> r(rowSize), g(rowSize), b(rowSize);
    e57::Data3DPointsData data;
    data.cartesianX = x.data();
    data.cartesianY = y.data();
    data.cartesianZ = z.data();
    data.colorRed = r.data();
    data.colorGreen = g.data();
    data.colorBlue = b.data();

    e57::CompressedVectorReader dataReader = reader.SetUpData3DPointsData(scanIndex, rowSize, data);

    int step = std::max(int(pointsSize / 100), 100);
    std::size_t nextProg = step;
    float iToProg = 100.f / pointsSize;
    TexturedMesh mesh;
    std::size_t size = 0;
    std::size_t index = 0;
    std::size_t nanCnt = 0;
    while ((size = dataReader.read()) > 0) {
        for (std::size_t i = 0; i < size; i++) {
            if (index == nextProg) {
                if (prog(index * iToProg)) {
                    return {};
                }
                nextProg += step;
            }
            ++index;
            if (!std::isfinite(x[i]) || !std::isfinite(y[i]) || !std::isfinite(z[i])) {
                nanCnt++;
                continue;
            }
            mesh.vertices.push_back(Pvl::Vec3f(x[i], y[i], z[i]));
            mesh.colors.push_back(Color(r[i], g[i], b[i]));
        }
    }
    std::cout << "Ignoring " << nanCnt << " NaN points" << std::endl;
    mesh.srs = Srs(Coords(tr.x, tr.y, tr.z));

    dataReader.close();
    return mesh;
}

} // namespace Mpcv
