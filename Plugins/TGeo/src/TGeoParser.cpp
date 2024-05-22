// This file is part of the Acts project.
//
// Copyright (C) 2020 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/TGeo/TGeoParser.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Plugins/TGeo/TGeoPrimitivesHelper.hpp"
#include "Acts/Utilities/VectorHelpers.hpp"

#include "RtypesCore.h"
#include "TCollection.h"
#include "TGeoBBox.h"
#include "TGeoNode.h"
#include "TGeoVolume.h"
#include "TObjArray.h"
#include "TObject.h"
#include <iostream>


// int i=0;
void Acts::TGeoParser::select(Acts::TGeoParser::State& state,
                              const Acts::TGeoParser::Options& options,
                              const TGeoMatrix& gmatrix) {
  // Volume is present
  if (state.volume != nullptr) {
    std::string volumeName = state.volume->GetName();
    // i++;
    // std::cout<<"=====i====="<<i<<std::endl;
    // std::cout<<"=====state.volume->GetName()====="<<state.volume->GetName()<<std::endl;
    // for(auto name : options.volumeNames){
    //   std::cout<<"=====options.volumeNames====="<<name<<std::endl;
    // }

    // If you are on branch, you stay on branch
    state.onBranch =
        state.onBranch ||
        TGeoPrimitivesHelper::match(options.volumeNames, volumeName.c_str());
    
    // Loop over the daughters and collect them
    auto daughters = state.volume->GetNodes();
    // Daughter node iteration
    TIter iObj(daughters);
    while (TObject* obj = iObj()) {
      TGeoNode* node = dynamic_cast<TGeoNode*>(obj);
      if (node != nullptr) {
        state.volume = nullptr;
        state.node = node;
        // std::cout<<"=====node->GetName()====="<<node->GetName()<<std::endl;
        select(state, options, gmatrix);
      }
    }
  } else if (state.node != nullptr) {
    // The node name for checking
    std::string nodeName = state.node->GetName();
    std::string nodeVolName = state.node->GetVolume()->GetName();


    // Get the matrix of the current node for positioning
    const TGeoMatrix* nmatrix = state.node->GetMatrix();
    TGeoHMatrix transform = TGeoCombiTrans(gmatrix) * TGeoCombiTrans(*nmatrix);
    std::string suffix = "_transform";
    transform.SetName((nodeName + suffix).c_str());
    // std::cout<<"transform = "<<transform.GetName()<<std::endl;
    const Double_t* translation = transform.GetTranslation();
    // std::cout<<"=====translation====="<<translation[0]<<" "<<translation[1]<<" "<<translation[2]<<std::endl;
    // Check if you had found the target node

    // for(auto name : options.targetNames){
    //   std::cout<<"=====target name====== "<<name<<std::endl;
    //   std::cout<<"=====nodeName====="<<nodeName.c_str()<<std::endl;
    //   std::cout<<"=====nodeVolName====="<<nodeVolName<<std::endl;
    // }
    if (state.onBranch &&
        TGeoPrimitivesHelper::match(options.targetNames, nodeVolName.c_str())) {
    // if (state.onBranch &&
    //     TGeoPrimitivesHelper::match(options.targetNames, nodeName.c_str())) {
      // Get the placement and orientation in respect to its mother
      const Double_t* rotation = transform.GetRotationMatrix();
      const Double_t* translation = transform.GetTranslation();
      // std::cout<<"=====rotation====="<<rotation[0]<<" "<<rotation[1]<<" "<<rotation[2]<<std::endl;
      // std::cout<<"=====translation====="<<translation[0]<<" "<<translation[1]<<" "<<translation[2]<<std::endl;
      // std::cout<<"=====nodeName====="<<nodeName.c_str()<<std::endl;
      // Create a eigen transform
      Vector3 t(options.unit * translation[0], options.unit * translation[1],
                options.unit * translation[2]);
      Vector3 cx(rotation[0], rotation[3], rotation[6]);
      Vector3 cy(rotation[1], rotation[4], rotation[7]);
      Vector3 cz(rotation[2], rotation[5], rotation[8]);
      auto etrf = TGeoPrimitivesHelper::makeTransform(cx, cy, cz, t);

      bool accept = true;
      if (!options.parseRanges.empty()) {
        auto shape =
            dynamic_cast<TGeoBBox*>(state.node->GetVolume()->GetShape());
        // It uses the bounding box of TGeoBBox
        // @TODO this should be replace by a proper TGeo to Acts::VolumeBounds
        // and vertices converision which would make a more appropriate parsomg
        double dx = options.unit * shape->GetDX();
        double dy = options.unit * shape->GetDY();
        double dz = options.unit * shape->GetDZ();
        for (auto x : std::vector<double>{-dx, dx}) {
          // std::cout<<"X"<<std::endl;
          for (auto y : std::vector<double>{-dy, dy}) {
            // std::cout<<"Y"<<std::endl;
            for (auto z : std::vector<double>{-dz, dz}) {
              Vector3 edge = etrf * Vector3(x, y, z);
              // std::cout<<"Z"<<std::endl;
              for (auto& check : options.parseRanges) {
                double val = VectorHelpers::cast(edge, check.first);
                // std::cout<<"=====nodeName====="<<nodeName.c_str()<<std::endl;
                // std::cout <<"val"<<val << std::endl;
                // std::cout<<"check.second.first"<<check.second.first<<std::endl;
                // std::cout<<"check.second.second"<<check.second.second<<std::endl;
                if (val < check.second.first || val > check.second.second) {
                  accept = false;

                  break;
                }
              }
            }
          }
        }
      }
      if (accept) {
        state.selectedNodes.push_back(
            {state.node, std::make_unique<TGeoHMatrix>(transform)});
      }
      state.node = nullptr;
    } else {
      // If it's not accepted, get the associated volume
      state.volume = state.node->GetVolume();
      // std::cout<<"=====node->GetName()====="<<state.node->GetName()<<std::endl;
      state.node = nullptr;
      // Set one further down
      select(state, options, transform);
    }
  }
  return;
}
