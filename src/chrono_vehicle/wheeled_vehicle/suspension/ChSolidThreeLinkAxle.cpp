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
// Base class for solid axle suspension with triangular and longitudinal guides.
//
//
// The suspension subsystem is modeled with respect to a right-handed frame,
// with X pointing towards the front, Y to the left, and Z up (ISO standard).
// The suspension reference frame is assumed to be always aligned with that of
// the vehicle.  When attached to a chassis, only an offset is provided.
//
// All point locations are assumed to be given for the left half of the
// suspension and will be mirrored (reflecting the y coordinates) to construct
// the right side.
//
// =============================================================================

#include "chrono/assets/ChColorAsset.h"
#include "chrono/assets/ChCylinderShape.h"
#include "chrono/assets/ChPointPointDrawing.h"

#include "chrono_vehicle/wheeled_vehicle/suspension/ChSolidThreeLinkAxle.h"

namespace chrono {
namespace vehicle {

// -----------------------------------------------------------------------------
// Static variables
// -----------------------------------------------------------------------------
const std::string ChSolidThreeLinkAxle::m_pointNames[] = {"SHOCK_A    ", "SHOCK_C    ", "SPRING_A   ",
                                                          "SPRING_C   ", "SPINDLE    ", "TRIANGLE_A ",
                                                          "TRIANGLE_C ", "LINK_A     ", "LINK_C     "};

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
ChSolidThreeLinkAxle::ChSolidThreeLinkAxle(const std::string& name) : ChSuspension(name) {}

ChSolidThreeLinkAxle::~ChSolidThreeLinkAxle() {
    auto sys = m_axleTube->GetSystem();
    if (sys) {
        sys->Remove(m_axleTube);
        ////sys->Remove(m_tierod);
        ////sys->Remove(m_axleTubeGuide);

        sys->Remove(m_triangleBody);
        sys->Remove(m_triangleRev);
        sys->Remove(m_triangleSph);

        for (int i = 0; i < 2; i++) {
            sys->Remove(m_linkBody[i]);
            sys->Remove(m_linkBodyToChassis[i]);
            sys->Remove(m_linkBodyToAxleTube[i]);

            sys->Remove(m_shock[i]);
            sys->Remove(m_spring[i]);
        }
    }
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void ChSolidThreeLinkAxle::Initialize(std::shared_ptr<ChChassis> chassis,
                                      std::shared_ptr<ChSubchassis> subchassis,
                                      std::shared_ptr<ChSteering> steering,
                                      const ChVector<>& location,
                                      double left_ang_vel,
                                      double right_ang_vel) {
    m_location = location;

    // Unit vectors for orientation matrices.
    ChVector<> u;
    ChVector<> v;
    ChVector<> w;
    ChMatrix33<> rot;

    // Express the suspension reference frame in the absolute coordinate system.
    ChFrame<> suspension_to_abs(location);
    suspension_to_abs.ConcatenatePreTransformation(chassis->GetBody()->GetFrame_REF_to_abs());

    // Transform the location of the axle body COM to absolute frame.
    ChVector<> axleCOM_local = getAxleTubeCOM();
    ChVector<> axleCOM = suspension_to_abs.TransformLocalToParent(axleCOM_local);

    // Calculate end points on the axle body, expressed in the absolute frame
    // (for visualization)
    ChVector<> outer_local(getLocation(SPINDLE));
    m_axleOuterL = suspension_to_abs.TransformPointLocalToParent(outer_local);
    outer_local.y() = -outer_local.y();
    m_axleOuterR = suspension_to_abs.TransformPointLocalToParent(outer_local);

    // Create and initialize the axle body.
    m_axleTube = std::shared_ptr<ChBody>(chassis->GetBody()->GetSystem()->NewBody());
    m_axleTube->SetNameString(m_name + "_axleTube");
    m_axleTube->SetPos(axleCOM);
    m_axleTube->SetRot(chassis->GetBody()->GetFrame_REF_to_abs().GetRot());
    m_axleTube->SetMass(getAxleTubeMass());
    m_axleTube->SetInertiaXX(getAxleTubeInertia());
    chassis->GetBody()->GetSystem()->AddBody(m_axleTube);

    // Transform all hardpoints to absolute frame.
    m_pointsL.resize(NUM_POINTS);
    m_pointsR.resize(NUM_POINTS);
    for (int i = 0; i < NUM_POINTS; i++) {
        ChVector<> rel_pos = getLocation(static_cast<PointId>(i));
        m_pointsL[i] = suspension_to_abs.TransformLocalToParent(rel_pos);
        rel_pos.y() = -rel_pos.y();
        m_pointsR[i] = suspension_to_abs.TransformLocalToParent(rel_pos);
    }

    // Fix the triangle body to the axle tube body
    ChVector<> pt_tri_axle = (m_pointsL[TRIANGLE_A] + m_pointsR[TRIANGLE_A]) / 2.0;
    ChVector<> pt_tri_chassis = (m_pointsL[TRIANGLE_C] + m_pointsR[TRIANGLE_C]) / 2.0;
    ChVector<> pt_tri_cog = (pt_tri_axle + pt_tri_chassis) / 2.0;
    m_triangle_left_point = m_pointsL[TRIANGLE_C];
    m_triangle_right_point = m_pointsR[TRIANGLE_C];
    m_triangle_sph_point = pt_tri_axle;

    // generate triangle body
    m_triangleBody = std::shared_ptr<ChBody>(chassis->GetBody()->GetSystem()->NewBody());
    m_triangleBody->SetNameString(m_name + "_triangleGuide");
    m_triangleBody->SetPos(pt_tri_cog);
    m_triangleBody->SetRot(chassis->GetBody()->GetFrame_REF_to_abs().GetRot());
    m_triangleBody->SetMass(getTriangleMass());
    m_triangleBody->SetInertiaXX(getTriangleInertia());
    chassis->GetBody()->GetSystem()->AddBody(m_triangleBody);

    // Create and initialize the revolute joint between chassis and triangle.
    ChCoordsys<> rev_csys(pt_tri_chassis, chassis->GetBody()->GetFrame_REF_to_abs().GetRot() * Q_from_AngX(CH_C_PI_2));
    m_triangleRev = chrono_types::make_shared<ChLinkLockRevolute>();
    m_triangleRev->SetNameString(m_name + "_revoluteTriangle");
    m_triangleRev->Initialize(m_triangleBody, chassis->GetBody(), rev_csys);
    chassis->GetBody()->GetSystem()->AddLink(m_triangleRev);

    // Create and initialize the spherical joint between axle tube and triangle.
    ChCoordsys<> sph_csys(pt_tri_axle, chassis->GetBody()->GetFrame_REF_to_abs().GetRot());
    m_triangleSph = chrono_types::make_shared<ChLinkLockSpherical>();
    m_triangleSph->SetNameString(m_name + "_sphericalTriangle");
    m_triangleSph->Initialize(m_triangleBody, m_axleTube, sph_csys);
    chassis->GetBody()->GetSystem()->AddLink(m_triangleSph);

    m_link_axleL = m_pointsL[LINK_A];
    m_link_axleR = m_pointsR[LINK_A];
    m_link_chassisL = m_pointsL[LINK_C];
    m_link_chassisR = m_pointsR[LINK_C];

    // Initialize left and right sides.
    std::shared_ptr<ChBody> scbeamL = (subchassis == nullptr) ? chassis->GetBody() : subchassis->GetBeam(LEFT);
    std::shared_ptr<ChBody> scbeamR = (subchassis == nullptr) ? chassis->GetBody() : subchassis->GetBeam(RIGHT);
    InitializeSide(LEFT, chassis->GetBody(), scbeamL, m_pointsL, left_ang_vel);
    InitializeSide(RIGHT, chassis->GetBody(), scbeamL, m_pointsR, right_ang_vel);
}

void ChSolidThreeLinkAxle::InitializeSide(VehicleSide side,
                                          std::shared_ptr<ChBodyAuxRef> chassis,
                                          std::shared_ptr<ChBody> scbeam,
                                          const std::vector<ChVector<>>& points,
                                          double ang_vel) {
    std::string suffix = (side == LEFT) ? "_L" : "_R";

    // Unit vectors for orientation matrices.
    ChVector<> u;
    ChVector<> v;
    ChVector<> w;
    ChMatrix33<> rot;

    // Chassis orientation (expressed in absolute frame)
    // Recall that the suspension reference frame is aligned with the chassis.
    ChQuaternion<> chassisRot = chassis->GetFrame_REF_to_abs().GetRot();

    // Create and initialize spindle body (same orientation as the chassis)
    m_spindle[side] = std::shared_ptr<ChBody>(chassis->GetSystem()->NewBody());
    m_spindle[side]->SetNameString(m_name + "_spindle" + suffix);
    m_spindle[side]->SetPos(points[SPINDLE]);
    m_spindle[side]->SetRot(chassisRot);
    m_spindle[side]->SetWvel_loc(ChVector<>(0, ang_vel, 0));
    m_spindle[side]->SetMass(getSpindleMass());
    m_spindle[side]->SetInertiaXX(getSpindleInertia());
    chassis->GetSystem()->AddBody(m_spindle[side]);

    // Create and initialize the revolute joint between axle tube and spindle.
    ChCoordsys<> rev_csys(points[SPINDLE], chassisRot * Q_from_AngAxis(CH_C_PI / 2.0, VECT_X));
    m_revolute[side] = chrono_types::make_shared<ChLinkLockRevolute>();
    m_revolute[side]->SetNameString(m_name + "_revolute" + suffix);
    m_revolute[side]->Initialize(m_spindle[side], m_axleTube, rev_csys);
    chassis->GetSystem()->AddLink(m_revolute[side]);

    // Create and initialize the shock damper
    m_shock[side] = chrono_types::make_shared<ChLinkTSDA>();
    m_shock[side]->SetNameString(m_name + "_shock" + suffix);
    m_shock[side]->Initialize(chassis, m_axleTube, false, points[SHOCK_C], points[SHOCK_A]);
    m_shock[side]->RegisterForceFunctor(getShockForceFunctor());
    chassis->GetSystem()->AddLink(m_shock[side]);

    // Create and initialize the spring
    m_spring[side] = chrono_types::make_shared<ChLinkTSDA>();
    m_spring[side]->SetNameString(m_name + "_spring" + suffix);
    m_spring[side]->Initialize(scbeam, m_axleTube, false, points[SPRING_C], points[SPRING_A], false,
                               getSpringRestLength());
    m_spring[side]->RegisterForceFunctor(getSpringForceFunctor());
    chassis->GetSystem()->AddLink(m_spring[side]);

    // Create and initialize the axle shaft and its connection to the spindle. Note that the
    // spindle rotates about the Y axis.
    m_axle[side] = chrono_types::make_shared<ChShaft>();
    m_axle[side]->SetNameString(m_name + "_axle" + suffix);
    m_axle[side]->SetInertia(getAxleInertia());
    m_axle[side]->SetPos_dt(-ang_vel);
    chassis->GetSystem()->Add(m_axle[side]);

    m_axle_to_spindle[side] = chrono_types::make_shared<ChShaftsBody>();
    m_axle_to_spindle[side]->SetNameString(m_name + "_axle_to_spindle" + suffix);
    m_axle_to_spindle[side]->Initialize(m_axle[side], m_spindle[side], ChVector<>(0, -1, 0));
    chassis->GetSystem()->Add(m_axle_to_spindle[side]);

    // Create and initialize spindle body (same orientation as the chassis)
    m_linkBody[side] = std::shared_ptr<ChBody>(chassis->GetSystem()->NewBody());
    m_linkBody[side]->SetNameString(m_name + "_linkBody" + suffix);
    m_linkBody[side]->SetPos((points[LINK_A] + points[LINK_C]) / 2.0);
    m_linkBody[side]->SetRot(chassisRot);
    m_linkBody[side]->SetMass(getLinkMass());
    m_linkBody[side]->SetInertiaXX(getLinkInertia());
    chassis->GetSystem()->AddBody(m_linkBody[side]);

    // Create and initialize the spherical joint between axle tube and link body.
    ChCoordsys<> sph1_csys(points[LINK_A], chassisRot);
    m_linkBodyToAxleTube[side] = chrono_types::make_shared<ChLinkLockSpherical>();
    m_linkBodyToAxleTube[side]->SetNameString(m_name + "_sphericalLinkToAxle" + suffix);
    m_linkBodyToAxleTube[side]->Initialize(m_linkBody[side], m_axleTube, sph1_csys);
    chassis->GetSystem()->AddLink(m_linkBodyToAxleTube[side]);

    // Create and initialize the spherical joint between axle tube and link body.
    v = Vcross(points[LINK_C] - points[LINK_A], ChVector<>(0, 1, 0));
    v.Normalize();
    w = points[LINK_C] - points[LINK_A];
    w.Normalize();
    u = Vcross(v, w);
    rot.Set_A_axis(u, v, w);

    // Create and initialize the universal joint between draglink and knuckle
    m_linkBodyToChassis[side] = chrono_types::make_shared<ChLinkUniversal>();
    m_linkBodyToChassis[side]->SetNameString(m_name + "_universalDraglink");
    m_linkBodyToChassis[side]->Initialize(m_linkBody[side], chassis, ChFrame<>(points[LINK_C], rot.Get_A_quaternion()));
    chassis->GetSystem()->AddLink(m_linkBodyToChassis[side]);
}

// -----------------------------------------------------------------------------
// Get the total mass of the suspension subsystem.
// -----------------------------------------------------------------------------
double ChSolidThreeLinkAxle::GetMass() const {
    return getAxleTubeMass() + getTriangleMass() + 2 * (getSpindleMass() + getLinkMass());
}

// -----------------------------------------------------------------------------
// Get the current COM location of the suspension subsystem.
// -----------------------------------------------------------------------------
ChVector<> ChSolidThreeLinkAxle::GetCOMPos() const {
    ChVector<> com(0, 0, 0);

    com += getAxleTubeMass() * m_axleTube->GetPos();

    com += getSpindleMass() * m_spindle[LEFT]->GetPos();
    com += getSpindleMass() * m_spindle[RIGHT]->GetPos();

    com += getLinkMass() * m_linkBody[LEFT]->GetPos();
    com += getLinkMass() * m_linkBody[RIGHT]->GetPos();

    com += getTriangleMass() * m_triangleBody->GetPos();

    return com / GetMass();
}

// -----------------------------------------------------------------------------
// Get the wheel track using the spindle local position.
// -----------------------------------------------------------------------------
double ChSolidThreeLinkAxle::GetTrack() {
    return 2 * getLocation(SPINDLE).y();
}

// -----------------------------------------------------------------------------
// Return current suspension forces
// -----------------------------------------------------------------------------
ChSuspension::Force ChSolidThreeLinkAxle::ReportSuspensionForce(VehicleSide side) const {
    ChSuspension::Force force;

    force.spring_force = m_spring[side]->GetForce();
    force.spring_length = m_spring[side]->GetLength();
    force.spring_velocity = m_spring[side]->GetVelocity();

    force.shock_force = m_shock[side]->GetForce();
    force.shock_length = m_shock[side]->GetLength();
    force.shock_velocity = m_shock[side]->GetVelocity();

    return force;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void ChSolidThreeLinkAxle::LogHardpointLocations(const ChVector<>& ref, bool inches) {
    double unit = inches ? 1 / 0.0254 : 1.0;

    for (int i = 0; i < NUM_POINTS; i++) {
        ChVector<> pos = ref + unit * getLocation(static_cast<PointId>(i));

        GetLog() << "   " << m_pointNames[i].c_str() << "  " << pos.x() << "  " << pos.y() << "  " << pos.z() << "\n";
    }
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void ChSolidThreeLinkAxle::LogConstraintViolations(VehicleSide side) {
    // TODO: Update this to reflect new suspension joints
    // Revolute joints

    {}
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void ChSolidThreeLinkAxle::AddVisualizationAssets(VisualizationType vis) {
    ChSuspension::AddVisualizationAssets(vis);

    if (vis == VisualizationType::NONE)
        return;

    AddVisualizationLink(m_axleTube, m_axleOuterL, m_axleOuterR, getAxleTubeRadius(), ChColor(0.7f, 0.7f, 0.7f));

    AddVisualizationLink(m_triangleBody, m_triangle_sph_point, m_triangle_left_point, getAxleTubeRadius() / 2.0,
                         ChColor(0.7f, 0.3f, 0.8f));
    AddVisualizationLink(m_triangleBody, m_triangle_sph_point, m_triangle_right_point, getAxleTubeRadius() / 2.0,
                         ChColor(0.7f, 0.3f, 0.8f));

    AddVisualizationLink(m_linkBody[LEFT], m_link_axleL, m_link_chassisL, getAxleTubeRadius() / 2.0,
                         ChColor(0.3f, 0.3f, 0.8f));
    AddVisualizationLink(m_linkBody[RIGHT], m_link_axleR, m_link_chassisR, getAxleTubeRadius() / 2.0,
                         ChColor(0.3f, 0.3f, 0.8f));

    // Add visualization for the springs and shocks
    m_spring[LEFT]->AddAsset(chrono_types::make_shared<ChPointPointSpring>(0.06, 150, 15));
    m_spring[RIGHT]->AddAsset(chrono_types::make_shared<ChPointPointSpring>(0.06, 150, 15));

    m_shock[LEFT]->AddAsset(chrono_types::make_shared<ChPointPointSegment>());
    m_shock[RIGHT]->AddAsset(chrono_types::make_shared<ChPointPointSegment>());
}

void ChSolidThreeLinkAxle::RemoveVisualizationAssets() {
    ChSuspension::RemoveVisualizationAssets();

    m_axleTube->GetAssets().clear();

    // m_triangle[LEFT]->GetAssets().clear();
    // m_triangle[RIGHT]->GetAssets().clear();

    m_spring[LEFT]->GetAssets().clear();
    m_spring[RIGHT]->GetAssets().clear();

    m_shock[LEFT]->GetAssets().clear();
    m_shock[RIGHT]->GetAssets().clear();
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void ChSolidThreeLinkAxle::AddVisualizationLink(std::shared_ptr<ChBody> body,
                                                const ChVector<> pt_1,
                                                const ChVector<> pt_2,
                                                double radius,
                                                const ChColor& color) {
    // Express hardpoint locations in body frame.
    ChVector<> p_1 = body->TransformPointParentToLocal(pt_1);
    ChVector<> p_2 = body->TransformPointParentToLocal(pt_2);

    auto cyl = chrono_types::make_shared<ChCylinderShape>();
    cyl->GetCylinderGeometry().p1 = p_1;
    cyl->GetCylinderGeometry().p2 = p_2;
    cyl->GetCylinderGeometry().rad = radius;
    body->AddAsset(cyl);

    auto col = chrono_types::make_shared<ChColorAsset>();
    col->SetColor(color);
    body->AddAsset(col);
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void ChSolidThreeLinkAxle::ExportComponentList(rapidjson::Document& jsonDocument) const {
    ChPart::ExportComponentList(jsonDocument);

    std::vector<std::shared_ptr<ChBody>> bodies;
    bodies.push_back(m_spindle[0]);
    bodies.push_back(m_spindle[1]);
    bodies.push_back(m_axleTube);
    ChPart::ExportBodyList(jsonDocument, bodies);

    std::vector<std::shared_ptr<ChShaft>> shafts;
    shafts.push_back(m_axle[0]);
    shafts.push_back(m_axle[1]);
    ChPart::ExportShaftList(jsonDocument, shafts);

    std::vector<std::shared_ptr<ChLink>> joints;
    joints.push_back(m_revolute[0]);
    joints.push_back(m_revolute[1]);
    ChPart::ExportJointList(jsonDocument, joints);

    std::vector<std::shared_ptr<ChLinkTSDA>> springs;
    springs.push_back(m_spring[0]);
    springs.push_back(m_spring[1]);
    springs.push_back(m_shock[0]);
    springs.push_back(m_shock[1]);
    ChPart::ExportLinSpringList(jsonDocument, springs);
}

void ChSolidThreeLinkAxle::Output(ChVehicleOutput& database) const {
    if (!m_output)
        return;

    std::vector<std::shared_ptr<ChBody>> bodies;
    bodies.push_back(m_spindle[0]);
    bodies.push_back(m_spindle[1]);
    bodies.push_back(m_axleTube);
    database.WriteBodies(bodies);

    std::vector<std::shared_ptr<ChShaft>> shafts;
    shafts.push_back(m_axle[0]);
    shafts.push_back(m_axle[1]);
    database.WriteShafts(shafts);

    std::vector<std::shared_ptr<ChLink>> joints;
    joints.push_back(m_revolute[0]);
    joints.push_back(m_revolute[1]);
    database.WriteJoints(joints);

    std::vector<std::shared_ptr<ChLinkTSDA>> springs;
    springs.push_back(m_spring[0]);
    springs.push_back(m_spring[1]);
    springs.push_back(m_shock[0]);
    springs.push_back(m_shock[1]);
    database.WriteLinSprings(springs);
}

}  // end namespace vehicle
}  // end namespace chrono
