// Copyright 2022 Anton Korobeynikov

// This file is part of Bandage

// Bandage is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

// Bandage is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with Bandage.  If not, see <http://www.gnu.org/licenses/>.

#include "graphlayout.h"
#include "graph/assemblygraphlist.h"

namespace layout::io {
    bool load(const QString &filename,
              GraphLayout &layout);
    bool loadLayoutList(const QString &filename,
              QSharedPointer<AssemblyGraphList> graphList);
    bool save(const QString &filename,
              const GraphLayout &layout);
    bool saveTSV(const QString &filename,
                 const GraphLayout &layout);

    bool saveGraphList(const QString &filename, QSharedPointer<AssemblyGraphList> graphList);
    bool saveGraphListTSV(const QString &filename, QSharedPointer<AssemblyGraphList> graphList);
}
