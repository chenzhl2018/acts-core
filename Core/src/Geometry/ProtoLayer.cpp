// This file is part of the Acts project.
//
// Copyright (C) 2018-2020 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "Acts/Geometry/ProtoLayer.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/DetectorElementBase.hpp"
#include "Acts/Geometry/Polyhedron.hpp"
#include "Acts/Utilities/Helpers.hpp"
#include "Acts/Surfaces/DiscTrapezoidBounds.hpp"

#include <cmath>

using Acts::VectorHelpers::perp;
using Acts::VectorHelpers::phi;

namespace Acts {

ProtoLayer::ProtoLayer(const GeometryContext& gctx,
                       const std::vector<const Surface*>& surfaces)
    : m_surfaces(surfaces) {
  measure(gctx, surfaces);
}

ProtoLayer::ProtoLayer(
    const GeometryContext& gctx,
    const std::vector<std::shared_ptr<const Surface>>& surfaces)
    : m_surfaces(unpack_shared_vector(surfaces)) {
  measure(gctx, m_surfaces);
}

double ProtoLayer::min(BinningValue bval, bool addenv) const {
  if(not m_surfaces.empty()){ 
    if (bval == binR and m_surfaces[0]->type() == Surface::Straw) {
      return m_rMin;
    }
  }  
  if (addenv) {
    return extent.min(bval) - envelope[bval][0u];
  }
  return extent.min(bval);
}

double ProtoLayer::max(BinningValue bval, bool addenv) const {
  if(not m_surfaces.empty()){ 
    if (bval == binR and m_surfaces[0]->type() == Surface::Straw) {
      return m_rMax;
    }
  }
  if (addenv) {
    return extent.max(bval) + envelope[bval][1u];
  }
  return extent.max(bval);
}

double ProtoLayer::medium(BinningValue bval, bool addenv) const {
  if(not m_surfaces.empty()){ 
    if (bval == binR and m_surfaces[0]->type() == Surface::Straw) {
      return (m_rMin + m_rMax) / 2;
    }
  }
  return 0.5 * (min(bval, addenv) + max(bval, addenv));
}

double ProtoLayer::range(BinningValue bval, bool addenv) const {
  if(not m_surfaces.empty()){ 
    if (bval == binR and m_surfaces[0]->type() == Surface::Straw) {
      return std::abs(m_rMin - m_rMax);
      return 1;
    }
  }
  return std::abs(max(bval, addenv) - min(bval, addenv));
}

std::ostream& ProtoLayer::toStream(std::ostream& sl) const {
  sl << "ProtoLayer with dimensions (min/max)" << std::endl;
  sl << extent.toString(); 
  return sl;
}

void ProtoLayer::measure(const GeometryContext& gctx,
                         const std::vector<const Surface*>& surfaces) {
  for (const auto& sf : surfaces) {
    Vector3 center = sf->center(gctx);

    // std::cout<<"1 m_rMin: " << m_rMin << std::endl;
    // std::cout<<"1 m_rMax: " << m_rMax << std::endl;
    // std::cout<<"1 center.z()"  << int(center.z())<< std::endl;
    if (int(center.z()) ==0)
    {
      ActsScalar radius = std::hypot(center.x(), center.y());
      // std::cout<<"2 m_rMin: " << m_rMin <<std::endl;
      // std::cout<<"2 m_rMax: " << m_rMax <<std::endl;
      // std::cout<<"2 center.z()"  << int(center.z())<< std::endl;
      if (radius < m_rMin) {
        m_rMin = radius;
      }
      if (radius > m_rMax) {
        m_rMax = radius;
      }
    }else{
      m_rMin = 644;
      m_rMax = 1004;
      
    }
    // std::cout<<int(center.x())<<"  "<<int(center.y())<<"  "<<int(center.z())<<std::endl;
    // std::cout<<(center.x())<<"  "<<(center.y())<<"  "<<(center.z())<<std::endl;
    auto sfPolyhedron = sf->polyhedronRepresentation(gctx, 1);
    const DetectorElementBase* element = sf->associatedDetectorElement();
    if (element != nullptr) {
      // Take the thickness in account if necessary
      double thickness = element->thickness();
      // We need a translation along and opposite half thickness
      Vector3 sfNormal = sf->normal(gctx, sf->center(gctx));
      std::vector<double> deltaT = {-0.5 * thickness, 0.5 * thickness};
      for (const auto& dT : deltaT) {
        Transform3 dtransform = Transform3::Identity();
        dtransform.pretranslate(dT * sfNormal);
        extent.extend(sfPolyhedron.extent(dtransform));
      }
      continue;
    }
    extent.extend(sfPolyhedron.extent());
  }
}

void ProtoLayer::add(const GeometryContext& gctx, const Surface& surface) {
  m_surfaces.push_back(&surface);
  measure(gctx, m_surfaces);
}

}  // namespace Acts
