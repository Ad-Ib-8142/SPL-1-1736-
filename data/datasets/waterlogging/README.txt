Dhaka GSI / processed / waterlogging

One observation table with stable IDs and consistent field names, UTF-8 CSV.
Coordinates: EPSG:4326 latitude/longitude; empty values mean unknown, not zero.
Only 20/54 records have road-feature point proxies. A proxy is NOT a surveyed position of flooding.
road_feature_id links to roads.csv @id and to derived road-point data. No road-segment geometry is supplied.
All waterlogged_reported=1: positive news reports only. Do not interpret missing/absent reports as dry (0).
All match_is_exact_flood_spot=0.
The time-window fields describe approximate context around news reporting. They do not prove
that waterlogging occurred continuously throughout the window. observation_time_local
is blank where the precise observation is not verified; two ~07:00 reports retain their
06:45-07:15 approximate windows instead of a fabricated exact time.
Timestamps are local Bangladesh (Asia/Dhaka), no UTC conversion or implicit timezone shift.
This dataset is for initial case-study checks, not sufficient for independently validated
road-level probabilistic waterlogging model accuracy.

Example Python standard library: 
  import csv
  with open("dhaka_waterlogging_observations_2024_DNCC_DSCC.csv", encoding="utf-8") as f:
      obs = list(csv.DictReader(f))

C++: read with a proper CSV parser (quoted text and commas occur inside fields).
