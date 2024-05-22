import os 

buildFilePath = "/data/zchen/QT/tgeo-plugin2/build/bin"
configFilePath = "/data/zchen/QT/trt-tracking-geometry/trt_geometry"
inputDir = "data/sim_trt"
outputDir = "data/reco_trt"
MatRecord_outDir = "data_mat"


runGeoSimple= f"{buildFilePath}/ActsExampleGeometryTGeo \
  -n 1 -l 0 \
  --geo-volume-loglevel 0 \
  --geo-tgeo-filename {configFilePath}/../InnerDetector.tgeo.root \
  --geo-tgeo-jsonconfig tgeo_atlas_id.json \
  --mat-output-file json_output/geometry-map7 \
  --output-json true \
  --mat-output-allmaterial true \
  --mat-output-sensitives 0"

runGeo= f"{buildFilePath}/ActsExampleGeometryTGeo \
  -n 1 -l 0 \
  --geo-volume-loglevel 0 \
  --geo-tgeo-filename {configFilePath}/../InnerDetector.tgeo.root \
  --geo-tgeo-jsonconfig tgeo_atlas_id.json \
  --mat-output-file json_output/ID-geometry-map6 \
  --output-json true \
  --mat-output-allmaterial true "

  # --output-obj \
  # --output-dir ID_TrackingGemetry \


runMatRecord = f"{buildFilePath}/ActsExampleMaterialRecordingGdml \
  --gdml-file={configFilePath}/../InnerDetecor.gdml \
  --output-root \
  --output-dir=data/mat/matRecord/test \
  --gen-eta -4:4 \
  --gen-nparticles 1000 \
  --gen-eta-uniform \
  -l 2 \
  -n 1000 \
  -j 1"

runMatMapping=f"{buildFilePath}/ActsExampleMaterialMappingTGeo \
  -j 1 -l 2 \
  --mat-mapping-read-surfaces 0 \
  --mat-input-type file --mat-input-file json_output/ID-geometry-map6.json \
  --mat-mapping-surfaces true \
  --mat-mapping-volumes true --mat-mapping-volume-stepsize 1 \
  --geo-tgeo-filename={configFilePath}/../InnerDetector.tgeo.root \
  --geo-tgeo-jsonconfig=tgeo_atlas_id.json \
  --prop-eta-range=-4:4 \
  --input-root=1 --input-file=data/mat/matRecord/test/geant4_material_tracks.root \
  --output-json --mat-output-file=data/mat/matMap/mat-id6 \
  --output-root --output-dir=data/mat/matMap "

runMatValidation=f"{buildFilePath}/ActsExampleMaterialValidationTGeo \
  -n 1000 -l 0  \
  --prop-resolve-passive 1 \
  --geo-tgeo-filename={configFilePath}/../InnerDetector.tgeo.root  \
  --geo-tgeo-jsonconfig=tgeo_atlas_id.json \
  --prop-z0-sigma 0 --prop-d0-sigma 0 \
  --mat-input-type file --mat-input-file=data/mat/matMap/mat-id6.json \
  --output-root --output-dir data/mat/matVal \
  --prop-eta-range=-4:4"

# os.system(runGeoSimple)
# os.system(runGeo)
# print(runMatRecord)
# os.system(runMatRecord)
# print(runMatMapping)
# os.system(runMatMapping)
# print("nohup " + runMatValidation + " &")
# os.system("nohup " + runMatValidation + " &")
os.system(runMatValidation)