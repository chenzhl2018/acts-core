// This file is part of the Acts project.
//
// Copyright (C) 2020 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "Acts/Geometry/ProtoLayerHelper.hpp"

#include "Acts/Geometry/Extent.hpp"
#include "Acts/Geometry/Polyhedron.hpp"
#include "Acts/Geometry/ProtoLayer.hpp"
#include "Acts/Surfaces/Surface.hpp"

#include <array>
#include <map>
#include <ostream>
#include <string>

std::vector<Acts::ProtoLayer> Acts::ProtoLayerHelper::protoLayers(
    const GeometryContext& gctx, const std::vector<const Surface*>& surfaces,
    const SortingConfig& sorting) const {
  std::vector<Acts::ProtoLayer> protoLayers;

  using SurfaceCluster = std::pair<Extent, std::vector<const Surface*>>;
  std::vector<SurfaceCluster> clusteredSurfaces;
  /// Helper function to find/create the cluster of surfaces where
  /// the Extent belongs to. In case none is found, a new one is inserted
  ///
  /// @param extent The test extent for finding the cluster
  ///
  /// @return the reference of the SurfaceCluster for insertion
  auto findCluster = [&](const Extent& extent) -> SurfaceCluster& {
    for (auto& cluster : clusteredSurfaces) {
      if (cluster.first.intersects(extent, sorting.first)) {
        return cluster;
      }
    }
    // No cluster found, let's create a new one
    clusteredSurfaces.push_back(SurfaceCluster(extent, {}));
    return clusteredSurfaces.back();
  };

  // Loop over surfaces and sort into clusters
  for (auto& sf : surfaces) {
    auto sfExtent = sf->polyhedronRepresentation(gctx, 1).extent();
    sfExtent.envelope()[sorting.first] = {sorting.second, sorting.second};
    // std::cout<<"sorting.second = " << sorting.second << std::endl;
    auto& sfCluster = findCluster(sfExtent);
    sfCluster.first.extend(sfExtent);
    sfCluster.second.push_back(sf);
  }
  // Loop over clusters and create ProtoLayer
  protoLayers.reserve(clusteredSurfaces.size());
  for (auto& clusters : clusteredSurfaces) {
    ACTS_VERBOSE("Creating ProtoLayer with " << clusters.second.size()
                                             << " surfaces.");
    protoLayers.push_back(ProtoLayer(gctx, clusters.second));
  }
  return protoLayers;
}

std::vector<Acts::ProtoLayer> Acts::ProtoLayerHelper::protoLayers(
    const GeometryContext& gctx, const std::vector<const Surface*>& surfaces,
    const std::vector<SortingConfig>& sortings) const {
  ACTS_DEBUG("Received " << surfaces.size() << " surfaces at input.");
  std::vector<std::vector<const Surface*>> sortSurfaces = {surfaces};
  std::cout << "sortings size " << sortings.size() << std::endl;
  std::cout << "sortSurfaces size " << sortSurfaces.size() << std::endl;
  for (const auto& sorting : sortings) {
    ACTS_VERBOSE("-> Sorting a set of " << sortSurfaces.size() << " in "
                                        << binningValueNames()[sorting.first]);
    std::vector<std::vector<const Surface*>> subSurfaces;
    for (const auto& ssurfaces : sortSurfaces) {
      ACTS_VERBOSE("-> Surfaces for this sorting step: " << ssurfaces.size());
      auto pLayers = protoLayers(gctx, ssurfaces, sorting);
      ACTS_VERBOSE("-> Resulted in " << pLayers.size() << " ProtoLayers.");
      for (const auto& pLayer : pLayers) {
        ACTS_VERBOSE("--> ProtoLayer contains " << pLayer.surfaces().size()
                                                << " surfaces.");
        subSurfaces.push_back(pLayer.surfaces());
      }
    }
    sortSurfaces = subSurfaces;
  }
  ACTS_DEBUG("Yielded " << sortSurfaces.size() << " at output.");

  std::vector<Acts::ProtoLayer> finalProtoLayers;

  for (const auto& ssurfaces : sortSurfaces) {
    finalProtoLayers.push_back(ProtoLayer(gctx, ssurfaces));
  }

  return finalProtoLayers;
}

std::vector<Acts::ProtoLayer> Acts::ProtoLayerHelper::protoLayers(
    const GeometryContext& gctx, const std::vector<const Surface*>& surfaces,
    const SortingConfig& sorting, int type, int minR, int minZ) const {
  ACTS_DEBUG("Received " << surfaces.size() << " surfaces at input.");
  ACTS_VERBOSE("-> Sorting a set of " << surfaces.size() << " in "
                                      << binningValueNames()[sorting.first]);
  std::map<int, std::vector<const Surface*>> subSurfaces;

  std::vector<const Surface*> sortSurfaces = surfaces;

  if (type == 0) {
    std::sort(sortSurfaces.begin(), sortSurfaces.end(),
              [&](const auto& a, const auto& b) {
                auto aR = std::hypot(a->center(gctx).x(), a->center(gctx).y());
                auto bR = std::hypot(b->center(gctx).x(), b->center(gctx).y());
                return aR < bR;
              });
    // int minR=560;
    for (const auto sf : sortSurfaces) {
      auto radius = std::hypot(sf->center(gctx).x(), sf->center(gctx).y());
      int bin = (radius - minR) / sorting.second;
      subSurfaces[bin].push_back(sf);
    }
  } else {
    // sorting by z
    std::sort(sortSurfaces.begin(), sortSurfaces.end(),
              [&](const auto& a, const auto& b) {
                auto aZ = abs(a->center(gctx).z());
                auto bZ = abs(b->center(gctx).z());
                return aZ < bZ;
              });
    // int minZ = 840;
    for (const auto sf : sortSurfaces) {
      int bin = (abs(sf->center(gctx).z()) - abs(minZ)) / sorting.second;
      subSurfaces[bin].push_back(sf);
      // std::cout << "sorting.second" <<sorting.second<<std::endl;
      // std::cout << sf->center(gctx).z()<<std::endl;
      // std::cout << "bin" <<bin<<std::endl;
    }
  }

  ACTS_DEBUG("Yielded " << subSurfaces.size() << " at output.");

  std::vector<Acts::ProtoLayer> finalProtoLayers;

  for (const auto& [i, ssurfaces] : subSurfaces) {
    finalProtoLayers.push_back(ProtoLayer(gctx, ssurfaces));
  }

  return finalProtoLayers;
}
