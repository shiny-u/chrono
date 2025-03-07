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
// Authors: Rainer Gericke, Radu Serban
// =============================================================================
//
// Steerable leaf-spring solid axle suspension constructed with data from file.
//
// =============================================================================

#ifndef SAE_TOEBAR_LEAFSPRINGAXLE_H
#define SAE_TOEBAR_LEAFSPRINGAXLE_H

#include "chrono_vehicle/ChApiVehicle.h"
#include "chrono_vehicle/wheeled_vehicle/suspension/ChSAEToeBarLeafspringAxle.h"

#include "chrono_thirdparty/rapidjson/document.h"

namespace chrono {
namespace vehicle {

/// @addtogroup vehicle_wheeled_suspension
/// @{

/// Steerable leaf-spring solid axle suspension constructed with data from file.
class CH_VEHICLE_API SAEToeBarLeafspringAxle : public ChSAEToeBarLeafspringAxle {
  public:
    SAEToeBarLeafspringAxle(const std::string& filename);
    SAEToeBarLeafspringAxle(const rapidjson::Document& d);
    ~SAEToeBarLeafspringAxle();

  protected:
    /// Return the mass of the knuckle body.
    virtual double getKnuckleMass() const override { return m_knuckleMass; }

    /// Return the mass of the tierod body.
    virtual double getTierodMass() const override { return m_tierodMass; }

    /// Return the mass of the draglink body.
    virtual double getDraglinkMass() const override { return m_draglinkMass; }

    virtual double getFrontLeafMass() const override { return m_frontleafMass; }
    virtual double getRearLeafMass() const override { return m_rearleafMass; }
    virtual double getClampMass() const override { return m_clampMass; }
    virtual double getShackleMass() const override { return m_clampMass; }

    /// Return the radius of the knuckle body (visualization only).
    virtual double getKnuckleRadius() const override { return m_knuckleRadius; }

    /// Return the radius of the tierod body (visualization only).
    virtual double getTierodRadius() const override { return m_tierodRadius; }

    /// Return the radius of the draglink body (visualization only).
    virtual double getDraglinkRadius() const override { return m_draglinkRadius; }

    /// Return the center of mass of the axle tube.
    virtual const ChVector<> getAxleTubeCOM() const override { return m_axleTubeCOM; }
    /// Return the radius of the spindle body (visualization only).
    virtual double getSpindleRadius() const override { return m_spindleRadius; }

    /// Return the width of the spindle body (visualization only).
    virtual double getSpindleWidth() const override { return m_spindleWidth; }

    /// Return the mass of the axle tube body.
    virtual double getAxleTubeMass() const override { return m_axleTubeMass; }
    /// Return the mass of the spindle body.
    virtual double getSpindleMass() const override { return m_spindleMass; }

    /// Return the radius of the axle tube body (visualization only).
    virtual double getAxleTubeRadius() const override { return m_axleTubeRadius; }

    /// Return the moments of inertia of the axle tube body.
    virtual const ChVector<>& getAxleTubeInertia() const override { return m_axleTubeInertia; }
    /// Return the moments of inertia of the spindle body.
    virtual const ChVector<>& getSpindleInertia() const override { return m_spindleInertia; }
    /// Return the moments of inertia of the knuckle body.
    virtual const ChVector<>& getKnuckleInertia() const override { return m_knuckleInertia; }
    /// Return the moments of inertia of the tierod body.
    virtual const ChVector<>& getTierodInertia() const override { return m_tierodInertia; }
    /// Return the moments of inertia of the draglink body.
    virtual const ChVector<>& getDraglinkInertia() const override { return m_draglinkInertia; }

    virtual const ChVector<>& getFrontLeafInertia() const override { return m_frontleafInertia; }
    virtual const ChVector<>& getRearLeafInertia() const override { return m_rearleafInertia; }
    virtual const ChVector<>& getClampInertia() const override { return m_clampInertia; }
    virtual const ChVector<>& getShackleInertia() const override { return m_shackleInertia; }

    /// Return the inertia of the axle shaft.
    virtual double getAxleInertia() const override { return m_axleInertia; }

    /// Return the free (rest) length of the spring element.
    virtual double getSpringRestLength() const override { return m_springRestLength; }

    /// Return the functor object for spring force.
    virtual std::shared_ptr<ChLinkTSDA::ForceFunctor> getSpringForceFunctor() const override { return m_springForceCB; }
    /// Return the functor object for shock force.
    virtual std::shared_ptr<ChLinkTSDA::ForceFunctor> getShockForceFunctor() const override { return m_shockForceCB; }

    virtual std::shared_ptr<ChLinkRotSpringCB::TorqueFunctor> getLatTorqueFunctorA() const override { return m_latRotSpringCBA; }
    virtual std::shared_ptr<ChLinkRotSpringCB::TorqueFunctor> getLatTorqueFunctorB() const override { return m_latRotSpringCBB; }

    virtual std::shared_ptr<ChLinkRotSpringCB::TorqueFunctor> getVertTorqueFunctorA() const override { return m_vertRotSpringCBA; }
    virtual std::shared_ptr<ChLinkRotSpringCB::TorqueFunctor> getVertTorqueFunctorB() const override { return m_vertRotSpringCBB; }

    virtual bool isLeftKnuckleActuated() override { return m_use_left_knuckle; }

    virtual std::shared_ptr<ChVehicleBushingData> getShackleBushingData() const override { return m_shackleBushingData; }
    virtual std::shared_ptr<ChVehicleBushingData> getClampBushingData() const override { return m_clampBushingData; }
    virtual std::shared_ptr<ChVehicleBushingData> getLeafspringBushingData() const override { return m_leafspringBushingData; }

  private:
    virtual const ChVector<> getLocation(PointId which) override { return m_points[which]; }

    virtual void Create(const rapidjson::Document& d) override;

    std::shared_ptr<ChLinkTSDA::ForceFunctor> m_springForceCB;
    std::shared_ptr<ChLinkTSDA::ForceFunctor> m_shockForceCB;

    std::shared_ptr<ChLinkRotSpringCB::TorqueFunctor> m_latRotSpringCBA;
    std::shared_ptr<ChLinkRotSpringCB::TorqueFunctor> m_latRotSpringCBB;

    std::shared_ptr<ChLinkRotSpringCB::TorqueFunctor> m_vertRotSpringCBA;
    std::shared_ptr<ChLinkRotSpringCB::TorqueFunctor> m_vertRotSpringCBB;

    ChVector<> m_points[NUM_POINTS];

    ////double m_damperDegressivityCompression;
    ////double m_damperDegressivityExpansion;

    double m_spindleMass;
    double m_axleTubeMass;
    double m_knuckleMass;
    double m_tierodMass;
    double m_draglinkMass;

    double m_frontleafMass;
    double m_rearleafMass;
    double m_clampMass;
    double m_shackleMass;

    double m_spindleRadius;
    double m_spindleWidth;
    double m_axleTubeRadius;
    double m_knuckleRadius;
    double m_tierodRadius;
    double m_draglinkRadius;

    double m_springRestLength;
    double m_axleInertia;

    ChVector<> m_spindleInertia;
    ChVector<> m_axleTubeInertia;
    ChVector<> m_axleTubeCOM;
    ChVector<> m_knuckleInertia;
    ChVector<> m_tierodInertia;
    ChVector<> m_draglinkInertia;

    ChVector<> m_frontleafInertia;
    ChVector<> m_rearleafInertia;
    ChVector<> m_clampInertia;
    ChVector<> m_shackleInertia;

    bool m_use_left_knuckle;

    std::shared_ptr<ChVehicleBushingData> m_shackleBushingData;
    std::shared_ptr<ChVehicleBushingData> m_clampBushingData;
    std::shared_ptr<ChVehicleBushingData> m_leafspringBushingData;
};

/// @} vehicle_wheeled_suspension

}  // end namespace vehicle
}  // end namespace chrono

#endif
