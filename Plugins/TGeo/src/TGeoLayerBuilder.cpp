// This file is part of the Acts project.
//
// Copyright (C) 2017-2018 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/TGeo/TGeoLayerBuilder.hpp"

#include "Acts/Geometry/Extent.hpp"
#include "Acts/Geometry/LayerCreator.hpp"
#include "Acts/Geometry/ProtoLayer.hpp"
#include "Acts/Geometry/ProtoLayerHelper.hpp"
#include "Acts/Plugins/TGeo/ITGeoDetectorElementSplitter.hpp"
#include "Acts/Plugins/TGeo/ITGeoIdentifierProvider.hpp"
#include "Acts/Plugins/TGeo/TGeoDetectorElement.hpp"
#include "Acts/Plugins/TGeo/TGeoParser.hpp"
#include "Acts/Plugins/TGeo/TGeoPrimitivesHelper.hpp"
#include "Acts/Utilities/Helpers.hpp"

#include <ostream>
#include <stdexcept>

#include "TGeoManager.h"
#include "TGeoMatrix.h"

namespace Acts {
class ISurfaceMaterial;
}  // namespace Acts

Acts::TGeoLayerBuilder::TGeoLayerBuilder(
    const Acts::TGeoLayerBuilder::Config& config,
    std::unique_ptr<const Logger> logger)
    : m_cfg(), m_logger(std::move(logger)) {
  setConfiguration(config);
}

Acts::TGeoLayerBuilder::~TGeoLayerBuilder() = default;

void Acts::TGeoLayerBuilder::setConfiguration(
    const Acts::TGeoLayerBuilder::Config& config) {
  m_cfg = config;
}

void Acts::TGeoLayerBuilder::setLogger(
    std::unique_ptr<const Logger> newLogger) {
  m_logger = std::move(newLogger);
}

const Acts::LayerVector Acts::TGeoLayerBuilder::negativeLayers(
    const GeometryContext& gctx) const {
  // @todo Remove this hack once the m_elementStore mess is sorted out
  auto mutableThis = const_cast<TGeoLayerBuilder*>(this);
  LayerVector nVector;
  mutableThis->buildLayers(gctx, nVector, -1);
  return nVector;
}

const Acts::LayerVector Acts::TGeoLayerBuilder::centralLayers(
    const GeometryContext& gctx) const {
  // @todo Remove this hack once the m_elementStore mess is sorted out
  auto mutableThis = const_cast<TGeoLayerBuilder*>(this);
  LayerVector cVector;
  mutableThis->buildLayers(gctx, cVector, 0);
  return cVector;
}

const Acts::LayerVector Acts::TGeoLayerBuilder::positiveLayers(
    const GeometryContext& gctx) const {
  // @todo Remove this hack once the m_elementStore mess is sorted out
  auto mutableThis = const_cast<TGeoLayerBuilder*>(this);
  LayerVector pVector;
  mutableThis->buildLayers(gctx, pVector, 1);
  return pVector;
}

void Acts::TGeoLayerBuilder::buildLayers(const GeometryContext& gctx,
                                         LayerVector& layers, int type) {
  // Bail out if you have no gGeoManager
  if (gGeoManager == nullptr) {
    ACTS_WARNING("No gGeoManager found - bailing out.");
    return;
  }

  using LayerSurfaceVector = std::vector<std::shared_ptr<const Surface>>;
  LayerSurfaceVector layerSurfaces;

  std::vector<LayerConfig> layerConfigs = m_cfg.layerConfigurations[type + 1];
  std::string layerType = m_layerTypes[type + 1];

  // Appropriate screen output
  std::string addonOutput = m_cfg.layerSplitToleranceR[type + 1] > 0.
                                ? std::string(", splitting in r")
                                : std::string("");
  addonOutput += m_cfg.layerSplitToleranceZ[type + 1] > 0.
                     ? std::string(", splitting in z")
                     : std::string("");
  addonOutput += std::string(".");

  // Screen output of the configuration
  ACTS_DEBUG(layerType << " layers : found " << layerConfigs.size()
                       << " configuration(s)" + addonOutput);

  // Helper function to fill the layer
  auto fillLayer = [&](const LayerSurfaceVector& lSurfaces,
                       const LayerConfig& lCfg,
                       unsigned int pl_id = 0) -> void {
    int nb0 = 0, nt0 = 0;
    bool is_autobinning = ((lCfg.binning0.size() == 1) &&
                           (std::get<int>(lCfg.binning0.at(0)) <= 0));
    if (!is_autobinning && std::get<int>(lCfg.binning0.at(pl_id)) <= 0) {
      throw std::invalid_argument(
          "Incorrect binning configuration found for loc0 protolayer #" +
          std::to_string(pl_id) +
          ". Layer is autobinned: No mixed binning (manual and auto) for loc0 "
          "possible between layers in a single subvolume. Quitting");
    }
    if (is_autobinning) {
      // Set binning by hand if nb0 > 0 and nb1 > 0
      nb0 = std::get<int>(lCfg.binning0.at(0));
      // Read the binning type
      nt0 = std::get<BinningType>(lCfg.binning0.at(0));
    } else if (pl_id < lCfg.binning0.size()) {
      // Set binning by hand if nb0 > 0 and nb1 > 0
      nb0 = std::get<int>(lCfg.binning0.at(pl_id));
    }

    int nb1 = 0, nt1 = 0;
    is_autobinning = (lCfg.binning1.size() == 1) &&
                     (std::get<int>(lCfg.binning1.at(0)) <= 0);
    if (!is_autobinning && std::get<int>(lCfg.binning1.at(pl_id)) <= 0) {
      throw std::invalid_argument(
          "Incorrect binning configuration found for loc1 protolayer #" +
          std::to_string(pl_id) +
          ". Layer is autobinned: No mixed binning (manual and auto) for loc1 "
          "possible between layers in a single subvolume. Quitting");
    }
    if (is_autobinning) {
      // Set binning by hand if nb0 > 0 and nb1 > 0
      nb1 = std::get<int>(lCfg.binning1.at(0));
      // For a binning type
      nt1 = std::get<BinningType>(lCfg.binning1.at(0));
    } else if (pl_id < lCfg.binning1.size()) {
      // Set binning by hand if nb0 > 0 and nb1 > 0
      nb1 = std::get<int>(lCfg.binning1.at(pl_id));
    }

    if (type == 0) {
      ProtoLayer pl(gctx, lSurfaces);
      ACTS_DEBUG("- creating CylinderLayer with "
                 << lSurfaces.size() << " surfaces at r = " << pl.medium(binR));

      pl.envelope[Acts::binR] = {lCfg.envelope.first, lCfg.envelope.second};
      pl.envelope[Acts::binZ] = {lCfg.envelope.second, lCfg.envelope.second};
      // std::cout<<"lCfg.envelope.first"<<lCfg.envelope.first<<std::endl;
      // std::cout<<"lCfg.envelope.second"<<lCfg.envelope.second<<std::endl;
      
      if (nb0 >= 0 && nb1 >= 0) {
        layers.push_back(
            m_cfg.layerCreator->cylinderLayer(gctx, lSurfaces, nb0, nb1, pl));
      } else {
        layers.push_back(
            m_cfg.layerCreator->cylinderLayer(gctx, lSurfaces, nt0, nt1, pl));
      }
    } else {
      ProtoLayer pl(gctx, lSurfaces);
      ACTS_DEBUG("- creating DiscLayer with "
                 << lSurfaces.size() << " surfaces at z = " << pl.medium(binZ));

      pl.envelope[Acts::binR] = {lCfg.envelope.first, lCfg.envelope.second};
      pl.envelope[Acts::binZ] = {lCfg.envelope.second, lCfg.envelope.second};
      std::cout<<"lCfg.envelope.first"<<lCfg.envelope.first<<std::endl;
      std::cout<<"lCfg.envelope.second"<<lCfg.envelope.second<<std::endl;
      if (nb0 >= 0 && nb1 >= 0) {
        layers.push_back(
            m_cfg.layerCreator->discLayer(gctx, lSurfaces, nb0, nb1, pl));
      } else {
        layers.push_back(
            m_cfg.layerCreator->discLayer(gctx, lSurfaces, nt0, nt1, pl));
      }
    }
  };

  for (auto layerCfg : layerConfigs) {
    ACTS_DEBUG("- layer configuration found for layer " << layerCfg.volumeName
                                                        << " with sensors ");
    for (auto& sensor : layerCfg.sensorNames) {
      ACTS_DEBUG("  - sensor: " << sensor);
    }
    if (!layerCfg.parseRanges.empty()) {
      for (const auto& pRange : layerCfg.parseRanges) {
        ACTS_DEBUG("- layer parsing restricted in "
                   << binningValueNames()[pRange.first] << " to ["
                   << pRange.second.first << "/" << pRange.second.second
                   << "].");
      }
    }
    if (!layerCfg.splitConfigs.empty()) {
      for (const auto& sConfig : layerCfg.splitConfigs) {
        ACTS_DEBUG("- layer splitting attempt in "
                   << binningValueNames()[sConfig.first] << " with tolerance "
                   << sConfig.second << ".");
      }
    }

    // Either pick the configured volume or take the top level volume
    TGeoVolume* tVolume =
        gGeoManager->FindVolumeFast(layerCfg.volumeName.c_str());

    // TObjArray* volumes = gGeoManager->GetListOfVolumes();
    // for (Int_t i = 0; i < volumes->GetEntries(); ++i) {
    //     TGeoVolume* volume = dynamic_cast<TGeoVolume*>(volumes->At(i));
    //     if (volume) {
    //         const char* volumeName = volume->GetName();
    //         printf("Volume name: %s\n", volumeName);
    //         std::cout << "Volume index: " << volume->GetNumber() << std::endl;
    //     }
    // }

    std::cout<<"======layerCfg.volumeName.c_str()======"<<layerCfg.volumeName.c_str()<<std::endl;
    if (tVolume == nullptr) {
      tVolume = gGeoManager->GetTopVolume();
      std::cout<<"=====tVolume====="<<tVolume->GetName()<<std::endl;
      ACTS_DEBUG("- search volume is TGeo top volume");
    } else {
      ACTS_DEBUG("- setting search volume to " << tVolume->GetName());
    }

    if (tVolume != nullptr) {
      TGeoParser::Options tgpOptions;
      std::cout<<"=====tVolume====="<<tVolume->GetName()<<std::endl;
      tgpOptions.volumeNames = {layerCfg.volumeName};
      tgpOptions.targetNames = layerCfg.sensorNames;
      for (const auto& name : layerCfg.sensorNames) {
        std::cout <<"=====sensor name====="<< name << std::endl;
      }
      tgpOptions.parseRanges = layerCfg.parseRanges;
      tgpOptions.unit = m_cfg.unit;
      TGeoParser::State tgpState;
      tgpState.volume = tVolume;

      ACTS_DEBUG("- applying  " << layerCfg.parseRanges.size()
                                << " search restrictions.");
      for (const auto& prange : layerCfg.parseRanges) {
        ACTS_VERBOSE(" - range " << binningValueNames()[prange.first]
                                 << " within [ " << prange.second.first << ", "
                                 << prange.second.second << "]");
        if(binningValueNames()[prange.first] == "binR"){
          minR=prange.second.first;
          std::cout<<"=====minR===="<<minR<<std::endl;
        }
        if(binningValueNames()[prange.first] == "binZ"){
          minZ=std::min(abs(prange.second.first),abs(prange.second.second));
          std::cout<<"=====minZ===="<<minZ<<std::endl;
        }        
      }

      if(type == 0){
        TGeoParser::select(tgpState, tgpOptions);
      }else if(type == 1){
        if (layerCfg.volumeName == "TRTEndcapWheelAB"){
          // TGeoNode *endcap = gGeoManager->GetTopVolume()->FindNode("TRT0x10ccbc810");
          // TGeoNode *endcap = gGeoManager->GetTopVolume()->FindNode("TRT0x114683ff0");
          TGeoNode *endcap = gGeoManager->GetTopVolume()->FindNode("TRT0x129ad9b50");
          const TGeoMatrix *ematrix = endcap->GetMatrix();
          TGeoParser::select(tgpState, tgpOptions,*ematrix);

        }else if(layerCfg.volumeName == "SCT_ForwardA"){
          // TGeoNode *endcap = gGeoManager->GetTopVolume()->FindNode("SCT0x112f7e8a0");
          TGeoNode *endcap = gGeoManager->GetTopVolume()->FindNode("SCT0x11e271950");
          const TGeoMatrix *ematrix = endcap->GetMatrix();
          TGeoParser::select(tgpState, tgpOptions,*ematrix);

        }else if (layerCfg.volumeName == "EndCap"){
          TGeoVolume* PVolume = gGeoManager->FindVolumeFast("Pixel");
          TGeoNode *endcap = PVolume->FindNode("EndCapA0x11e524240");
          const TGeoMatrix *ematrix = endcap->GetMatrix();
          TGeoParser::select(tgpState, tgpOptions,*ematrix);

        }else{
          TGeoParser::select(tgpState, tgpOptions);

        }

      }else if(type == -1){      
        if (layerCfg.volumeName == "TRTEndcapWheelAB"){
        // TGeoNode *endcap = gGeoManager->GetTopVolume()->FindNode("TRT0x10e3c6140");
        // TGeoNode *endcap = gGeoManager->GetTopVolume()->FindNode("TRT0x11469d650");
        TGeoNode *endcap = gGeoManager->GetTopVolume()->FindNode("TRT0x12cbeb4b0");
          const TGeoMatrix *ematrix = endcap->GetMatrix();
          TGeoParser::select(tgpState, tgpOptions,*ematrix);

        }else if(layerCfg.volumeName == "SCT_ForwardC"){
          // TGeoNode *endcap = gGeoManager->GetTopVolume()->FindNode("SCT0x112f7e8a0");
          TGeoNode *endcap = gGeoManager->GetTopVolume()->FindNode("SCT0x11f6e52a0");
          const TGeoMatrix *ematrix = endcap->GetMatrix();
          TGeoParser::select(tgpState, tgpOptions,*ematrix);

        }else if (layerCfg.volumeName == "EndCap"){
          TGeoVolume* PVolume = gGeoManager->FindVolumeFast("Pixel");
          TGeoNode *endcap = PVolume->FindNode("EndCapC0x11e577e60");
          const TGeoMatrix *ematrix = endcap->GetMatrix();
          TGeoParser::select(tgpState, tgpOptions,*ematrix);

        }else{
          TGeoParser::select(tgpState, tgpOptions);

        }
      }
      
      ACTS_DEBUG("- number of selected nodes found : "
                 << tgpState.selectedNodes.size());

      for (auto& snode : tgpState.selectedNodes) {
        auto identifier =
            m_cfg.identifierProvider != nullptr
                ? m_cfg.identifierProvider->identify(gctx, *snode.node)
                : Identifier();

        auto tgElement =
            m_cfg.elementFactory(identifier, *snode.node, *snode.transform,
                                 layerCfg.localAxes, m_cfg.unit, nullptr);

        std::vector<std::shared_ptr<const Acts::TGeoDetectorElement>>
            tgElements =
                (m_cfg.detectorElementSplitter == nullptr)
                    ? std::vector<std::shared_ptr<
                          const Acts::TGeoDetectorElement>>{tgElement}
                    : m_cfg.detectorElementSplitter->split(gctx, tgElement);

        for (const auto& tge : tgElements) {
          m_elementStore.push_back(tge);
          layerSurfaces.push_back(tge->surface().getSharedPtr());
        }
      }

      ACTS_DEBUG("- created TGeoDetectorElements : " << layerSurfaces.size());
      if(m_cfg.protoLayerHelper != nullptr) ACTS_DEBUG("protoLayerHelper not null");
      if(!layerCfg.splitConfigs.empty()) ACTS_DEBUG("layer splitConfigs not empty");

      if (m_cfg.protoLayerHelper != nullptr && !layerCfg.splitConfigs.empty()) {
        
        std::vector<Acts::ProtoLayer> protoLayers;

        if(type==0){
          protoLayers = m_cfg.protoLayerHelper->protoLayers(
              //gctx, unpack_shared_vector(layerSurfaces), layerCfg.splitConfigs);
              gctx, unpack_shared_vector(layerSurfaces), layerCfg.splitConfigs[0], type,minR,0);
          std::cout<<"layerCfg.splitConfigs[0]"<<layerCfg.splitConfigs[0].first<<std::endl;
          std::cout<<"layerCfg.splitConfigs[0]"<<layerCfg.splitConfigs[0].second<<std::endl;
        }else if(type==1 || type==-1){
          protoLayers = m_cfg.protoLayerHelper->protoLayers(
              gctx, unpack_shared_vector(layerSurfaces), layerCfg.splitConfigs[0], type, 0, minZ);
          // std::cout<<"layerCfg.splitConfigs[0]"<<layerCfg.splitConfigs[0].first<<std::endl;
          // std::cout<<"layerCfg.splitConfigs[0]"<<layerCfg.splitConfigs[0].second<<std::endl;
              // gctx, unpack_shared_vector(layerSurfaces), layerCfg.splitConfigs[0]);
          std::cout <<" splitting into " << protoLayers.size() << " layers." << std::endl;
        }

        ACTS_DEBUG("- splitting into " << protoLayers.size() << " layers.");

        // Number of options mismatch and has not been configured for
        // auto-binning
        const bool is_loc0_n_config =
            layerCfg.binning0.size() == protoLayers.size();
        const bool is_loc0_autobinning =
            (layerCfg.binning0.size() == 1) &&
            (std::get<int>(layerCfg.binning0.at(0)) <= 0);
        const bool is_loc1_n_config =
            layerCfg.binning1.size() == protoLayers.size();
        const bool is_loc1_autobinning =
            (layerCfg.binning1.size() == 1) &&
            (std::get<int>(layerCfg.binning1.at(0)) <= 0);
        if ((!is_loc0_n_config && !is_loc0_autobinning) ||
            (!is_loc1_n_config && !is_loc1_autobinning)) {
          throw std::invalid_argument(
              "Incorrect binning configuration found: Number of configurations "
              "does not match number of protolayers in subvolume " +
              layerCfg.volumeName + ". Quitting.");
        }
        unsigned int layer_id = 0;
        for (auto& pLayer : protoLayers) {
          layerSurfaces.clear();

          for (const auto& lsurface : pLayer.surfaces()) {
            layerSurfaces.push_back(lsurface->getSharedPtr());
            // std::cout<<"=====lsurface====="<<lsurface->geometryId()<<std::endl;
          }
          fillLayer(layerSurfaces, layerCfg, layer_id);
          layer_id++;
          std::cout<<" layer_id " <<layer_id<<std::endl;
        }
      } else {
        fillLayer(layerSurfaces, layerCfg);
      }
    }
  }
  return;
}

std::shared_ptr<Acts::TGeoDetectorElement>
Acts::TGeoLayerBuilder::defaultElementFactory(
    const Identifier& identifier, const TGeoNode& tGeoNode,
    const TGeoMatrix& tGeoMatrix, const std::string& axes, double scalor,
    std::shared_ptr<const Acts::ISurfaceMaterial> material) {
  return std::make_shared<TGeoDetectorElement>(
      identifier, tGeoNode, tGeoMatrix, axes, scalor, std::move(material));
}
