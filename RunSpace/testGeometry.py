import os 

buildFilePath = "/home/xiaocong/Software/acts/trtActs/build/bin"
configFilePath = "./"
inputDir = "data/sim_trt"
outputDir = "data/reco_trt"
MatRecord_outDir = "data_mat"


runGeoSimple= f"{buildFilePath}/ActsExampleGeometryTGeo \
  -n 1 -l 0 \
  --geo-volume-loglevel 0 \
  --geo-tgeo-filename {configFilePath}/InnerDetector.tgeo.root \
  --geo-tgeo-jsonconfig tgeo_atlas_id.json \
  --mat-output-file json_output/geometry-map \
  --output-json true \
  --mat-output-allmaterial true \
  --mat-output-sensitives 0"


runGeoSimulation= f"{buildFilePath}/ActsExampleFatrasTGeo \
  --geo-tgeo-filename {configFilePath}/InnerDetector.tgeo.root \
  --geo-tgeo-jsonconfig tgeo_atlas_id.json \
  --gen-eta -2.5:2.5 \
  --output-root 1 -l 0 -j 1 --events 2000"

os.system(runGeoSimple)
#os.system(runGeoSimulation)
