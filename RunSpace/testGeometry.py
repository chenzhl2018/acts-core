import os 

buildFilePath = "/home/xiaocong/Software/trtActs/build/bin"
configFilePath = "./"
inputDir = "data/sim_trt"
outputDir = "data/reco_trt"
MatRecord_outDir = "data_mat"


runGeoSimple= f"{buildFilePath}/ActsExampleGeometryTGeo \
  -n 1 -l 0 \
  --geo-volume-loglevel 0 \
  --geo-tgeo-filename {configFilePath}/InnerDetector.tgeo.root \
  --geo-tgeo-jsonconfig tgeo_atlas_id_test.json \
  --mat-output-file json_output/geometry-map \
  --output-json true \
  --mat-output-allmaterial true \
  --mat-output-sensitives 0"


runGeoSimulation= f"{buildFilePath}/ActsExampleFatrasTGeo \
  --geo-tgeo-filename {configFilePath}/InnerDetector.tgeo.root \
  --geo-tgeo-jsonconfig tgeo_atlas_id_test.json \
  --output-root 1 -l 0 -j 1 --events 1000"

os.system(runGeoSimple)
#os.system(runGeoSimulation)
