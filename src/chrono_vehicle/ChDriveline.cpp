// =============================================================================
// PROJECT CHRONO - http://projectchrono.org
//
// Copyright (c) 2014 projectchrono.org
// All rights reserved.
//
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file at the top level of the distribution and at
// http://projectchrono.org/license-chrono.txt.
//
// =============================================================================
// Authors: Radu Serban
// =============================================================================
//
// Base class for a vehicle driveline.
//
// =============================================================================

#include "chrono/physics/ChSystem.h"
#include "chrono_vehicle/ChDriveline.h"

namespace chrono {
namespace vehicle {

ChDriveline::ChDriveline(const std::string& name) : ChPart(name) {}

ChDriveline::~ChDriveline() {
    if (!m_driveshaft)
        return;
    auto sys = m_driveshaft->GetSystem();
    if (sys) {
        sys->Remove(m_driveshaft);
    }
}

}  // end namespace vehicle
}  // end namespace chrono
