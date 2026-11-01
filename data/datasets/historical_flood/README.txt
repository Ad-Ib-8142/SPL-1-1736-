Historical flood polygons are clipped separately by year. Do not merge years without preserving year and classification methodology.
Each year has:
- GeoPackage: projected EPSG:32646 polygon geometry.
- _attributes.csv: attributes without geometry.
- _WKT.csv: attributes plus geometry_wkt and geometry_crs=EPSG:32646 for C++ parsing.
