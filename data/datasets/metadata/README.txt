Dhaka GSI — University Software Project Lab
Urban flood prediction module — Dhaka, Bangladesh

STUDY AREA
The working study area is the union of the DNCC and DSCC polygons supplied in DCC.zip.
The user approved this as a PROVISIONAL boundary.
Embedded source metadata records an ArcGIS Union operation dated 2018-03-12 and metadata creation date 2023-09-28. The current official currency of the boundary is not verified.
Do not describe outputs as clipped to a currently verified official boundary.

ORIGINALS
data/raw/ contains unchanged copies of all uploaded source files, including DCC.zip and bd_shp.zip.
The uploaded originals outside this workspace were not modified.

PROCESSED DATA
data/processed/ contains spatially clipped and filtered outputs. All clipping uses the provisional combined DNCC+DSCC polygon.
metadata/ contains boundary, inventory, cleaning log, CRS register and data dictionary.
The `roads` folder now contains 13,873 coordinate-filtered road-feature points from roads.csv. These are not connected road lines and cannot support routing or road-network distance calculations. The `flood_events` folder remains empty because no dated flood-event catalogue was supplied.

IMPORTANT LIMITATIONS
- HOTOSM is volunteered OpenStreetMap data and is not a surveyed drainage engineering network.
- OSM ways in CSVs have representative coordinates only; use GeoJSON/GPKG line geometries for network operations.
- Weather has 24 retained IDs but only 5 unique coordinate pairs; do not count IDs as 24 independent spatial stations.
- humidity.csv actually contains precipitation.
- Soil moisture depth columns are identical.
- DEM source vertical datum/units are not explicit in the GeoTIFF tags.
- Historical flood classification gaps and 2007 duplicate IDs remain documented, not silently repaired.

Additional processed folders: soil/, weather_aux/ and waste_infrastructure/ are included because the uploaded data contain these variables, even though they were not in the initial suggested directory list.

Download packaging note: the processed archive includes the raw folder structure and raw-source checksum manifest, but omits the large raw source binaries/CSVs. The original uploads remain unchanged and available separately in the conversation; the working project raw directory contains their copies.


NEW UPLOADS (30 October 2026)
- DCC(1).zip: compared with DCC.zip; same two polygons, attributes and geometries. Kept as a separate unchanged raw file; existing boundary was not replaced.
- roads.csv: 14,516 records; 13,873 coordinate points inside provisional DNCC+DSCC; 643 outside. The file does not contain road line geometries or connectivity. See data/processed/roads/README.txt.
