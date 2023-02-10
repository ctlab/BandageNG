// Copyright 2022 Anton Korobeynikov

// This file is part of Bandage-NG

// Bandage is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// Bandage is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with Bandage.  If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include "featurenodecolorer.h"

class FeatureUniformNodeColorer : public IFeatureNodeColorer {
public:
    using IFeatureNodeColorer::IFeatureNodeColorer;

    QColor get(const GraphicsItemFeatureNode *node) override;
    [[nodiscard]] const char* name() const override { return "Uniform color"; };
};

class FeatureCustomNodeColorer : public IFeatureNodeColorer {
public:
    using IFeatureNodeColorer::IFeatureNodeColorer;

    QColor get(const GraphicsItemFeatureNode *node) override;
    [[nodiscard]] const char* name() const override { return "Custom colors"; };
};

class FeatureClassNodeColorer : public IFeatureNodeColorer {
public:
    using IFeatureNodeColorer::IFeatureNodeColorer;

    QColor get(const GraphicsItemFeatureNode *node) override;
    [[nodiscard]] const char* name() const override { return "Class colors"; };
};

class FeatureBlastSolidNodeColorer : public IFeatureNodeColorer {
public:
    using IFeatureNodeColorer::IFeatureNodeColorer;

    QColor get(const GraphicsItemFeatureNode *node) override;
    [[nodiscard]] const char* name() const override { return "Blast solid colors"; };
};

class FeatureBlastClassNodeColorer : public IFeatureNodeColorer {
public:
    using IFeatureNodeColorer::IFeatureNodeColorer;

    QColor get(const GraphicsItemFeatureNode *node) override;
    [[nodiscard]] const char* name() const override { return "Blast feature class colors"; };
};
