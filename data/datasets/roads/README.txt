Road dataset — coordinate-filtered road-feature points

Source: roads.csv (14,516 records), fields @id, @lat, @lon, name, highway.
CRS interpretation: @lat/@lon treated as WGS84 (EPSG:4326) based on field names and coordinate ranges; source does not declare CRS.
Filtering: retained 13,873 point records whose coordinates fall within the provisional DNCC+DSCC polygon; 643 records outside were excluded.
Highway classes retained:
- residential: 11,640
- tertiary: 867
- secondary: 508
- primary: 488
- primary_link: 155
- secondary_link: 142
- tertiary_link: 73

Outputs:
- roads_coordinate_records_DNCC_DSCC.csv: filtered attributes.
- roads_feature_points_DNCC_DSCC.geojson: point geometries in EPSG:4326.
- roads_feature_points_DNCC_DSCC.gpkg: point layer in EPSG:4326.
- roads_feature_points_DNCC_DSCC_UTM46N_WKT.csv: point WKT in EPSG:32646, metres.

IMPORTANT:
The supplied CSV contains one coordinate per record and no road line geometry, node sequences, topology or connectivity. These outputs are road-feature points only. They must NOT be used as a connected road network, for road lengths, routing or network-distance calculations. A true OSM roads LineString/MultiLineString source is still required for those tasks.
Names are missing for 8,869 of the 13,873 retained records. Four duplicate coordinate pairs and no duplicate @id values were found in the original CSV. Missing names remain missing.
