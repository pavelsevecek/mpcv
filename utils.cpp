#include "utils.h"

namespace Mpcv {

QDir& saveFileDialogInitialDir() {
    static QDir initialDir(".");
    return initialDir;
}

QDir& openFileDialogInitialDir() {
    static QDir initialDir(".");
    return initialDir;
}

} // namespace Mpcv
