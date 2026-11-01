
from pathlib import Path
from collections import defaultdict
import csv
import json

from shapely.geometry import Point, shape
from shapely.ops import unary_union

# =====================================================
# 1. PROJECT PATHS
# =====================================================

ROOT = Path(r"H:\spl1")

BOUNDARY_FILE = (
    ROOT / "data" / "datasets" / "metadata" /
    "DNCC_DSCC_Provisional_Boundary.geojson"
)

ADJUSTED_FOLDER = (
    ROOT / "output" / "weather_adjusted"
)

REPORT_FILE = (
    ADJUSTED_FOLDER / "dhaka_boundary_verification.csv"
)

EXPECTED_POINTS = 30
EXPECTED_DATASETS = 9

# =====================================================
# 2. LOAD DNCC + DSCC BOUNDARY
# =====================================================

def load_boundary(path):

    if not path.exists():
        raise FileNotFoundError(
            f"Boundary file not found: {path}"
        )

    with open(path, "r", encoding="utf-8-sig") as f:
        data = json.load(f)

    geometries = []

    if data["type"] == "FeatureCollection":

        for feature in data["features"]:
            if feature.get("geometry"):
                geometries.append(
                    shape(feature["geometry"])
                )

    elif data["type"] == "Feature":

        geometries.append(shape(data["geometry"]))

    else:

        geometries.append(shape(data))

    boundary = unary_union(geometries)

    if boundary.is_empty:
        raise ValueError("Boundary geometry is empty.")

    if not boundary.is_valid:
        raise ValueError(
            "Boundary geometry is invalid. "
            "Do not automatically repair it without inspection."
        )

    # GeoJSON coordinates are expected in WGS84:
    # longitude, latitude.

    minx, miny, maxx, maxy = boundary.bounds

    if not (
        85 <= minx <= 95 and
        85 <= maxx <= 95 and
        15 <= miny <= 30 and
        15 <= maxy <= 30
    ):
        raise ValueError(
            "Boundary coordinates do not look like "
            "Dhaka longitude/latitude (WGS84). "
            "Check the GeoJSON CRS."
        )

    print("\nBOUNDARY LOADED SUCCESSFULLY")
    print("File:", path.name)
    print("Longitude range:", minx, "to", maxx)
    print("Latitude range:", miny, "to", maxy)

    return boundary


# =====================================================
# 3. FIND CSV COLUMNS
# =====================================================

def find_column(headers, candidates):

    lookup = {
        h.strip().lower(): h
        for h in headers
    }

    for candidate in candidates:

        if candidate in lookup:
            return lookup[candidate]

    return None


# =====================================================
# 4. VERIFY ONE WEATHER DATASET
# =====================================================

def verify_dataset(csv_path, boundary):

    print("\n" + "=" * 55)
    print("CHECKING:", csv_path.name)

    coordinate_map = defaultdict(set)

    total_rows = 0
    invalid_rows = 0

    with open(
        csv_path,
        "r",
        newline="",
        encoding="utf-8-sig"
    ) as f:

        reader = csv.DictReader(f)

        if not reader.fieldnames:
            print("FAIL: Empty CSV.")
            return None

        id_col = find_column(
            reader.fieldnames,
            ["target_id", "requested_location_id",
             "target_location_id"]
        )

        lat_col = find_column(
            reader.fieldnames,
            ["target_lat", "target_latitude",
             "requested_latitude"]
        )

        lon_col = find_column(
            reader.fieldnames,
            ["target_lon", "target_long",
             "target_longitude", "requested_longitude"]
        )

        if not all([id_col, lat_col, lon_col]):

            print("FAIL: Required target-coordinate columns missing.")
            print("Available columns:", reader.fieldnames)

            return None

        for row in reader:

            total_rows += 1

            try:

                target_id = row[id_col].strip()

                lat = float(row[lat_col])
                lon = float(row[lon_col])

                if not target_id:
                    raise ValueError("Missing target ID")

                if not (-90 <= lat <= 90):
                    raise ValueError("Invalid latitude")

                if not (-180 <= lon <= 180):
                    raise ValueError("Invalid longitude")

                coordinate_map[target_id].add(
                    (round(lat, 7), round(lon, 7))
                )

            except (ValueError, TypeError, KeyError):
                invalid_rows += 1

    all_coordinates = set()
    conflicting_ids = []

    for target_id, coordinates in coordinate_map.items():

        all_coordinates.update(coordinates)

        if len(coordinates) != 1:
            conflicting_ids.append(target_id)

    outside_points = []

    for lat, lon in sorted(all_coordinates):

        point = Point(lon, lat)

        # covers() includes points on the boundary.
        if not boundary.covers(point):

            outside_points.append((lat, lon))

    correct_ids = len(coordinate_map) == EXPECTED_POINTS

    correct_coordinates = (
        len(all_coordinates) == EXPECTED_POINTS
    )

    no_conflicts = len(conflicting_ids) == 0

    no_outside = len(outside_points) == 0

    no_invalid = invalid_rows == 0

    passed = all([
        correct_ids,
        correct_coordinates,
        no_conflicts,
        no_outside,
        no_invalid
    ])

    print("Total records:", total_rows)

    print(
        "Target IDs:",
        len(coordinate_map),
        "/",
        EXPECTED_POINTS
    )

    print(
        "Unique coordinates:",
        len(all_coordinates),
        "/",
        EXPECTED_POINTS
    )

    print("Outside boundary:", len(outside_points))
    print("Invalid records:", invalid_rows)

    print("Conflicting target IDs:", conflicting_ids)

    if outside_points:

        print("\nCOORDINATES OUTSIDE DNCC + DSCC:")

        for lat, lon in outside_points:
            print(f"Latitude: {lat}, Longitude: {lon}")

    print("\nDATASET RESULT:", "PASS" if passed else "FAIL")

    return {
        "file": csv_path.name,
        "records": total_rows,
        "target_ids": len(coordinate_map),
        "unique_coordinates": len(all_coordinates),
        "outside_points": len(outside_points),
        "invalid_records": invalid_rows,
        "conflicting_ids": len(conflicting_ids),
        "result": "PASS" if passed else "FAIL",
        "coordinates": all_coordinates
    }


# =====================================================
# 5. VERIFY ALL ADJUSTED WEATHER DATASETS
# =====================================================

def main():

    print("=" * 55)
    print("DHAKA GSI - BOUNDARY VERIFICATION")
    print("=" * 55)

    if not ADJUSTED_FOLDER.exists():

        raise FileNotFoundError(
            f"Output folder not found: {ADJUSTED_FOLDER}"
        )

    boundary = load_boundary(BOUNDARY_FILE)

    files = sorted(
        ADJUSTED_FOLDER.glob("*_30.csv")
    )

    if not files:

        raise FileNotFoundError(
            "No adjusted *_30.csv files found."
        )

    results = []

    reference_coordinates = None

    for csv_path in files:

        result = verify_dataset(
            csv_path,
            boundary
        )

        if result is None:
            continue

        coordinates = result.pop("coordinates")

        if reference_coordinates is None:

            reference_coordinates = coordinates

        elif coordinates != reference_coordinates:

            print(
                "WARNING: Coordinates differ "
                "from the first dataset."
            )

            result["result"] = "FAIL"

        results.append(result)

    # =================================================
    # 6. SAVE VERIFICATION REPORT
    # =================================================

    if results:

        with open(
            REPORT_FILE,
            "w",
            newline="",
            encoding="utf-8"
        ) as f:

            fields = [
                "file",
                "records",
                "target_ids",
                "unique_coordinates",
                "outside_points",
                "invalid_records",
                "conflicting_ids",
                "result"
            ]

            writer = csv.DictWriter(
                f,
                fieldnames=fields
            )

            writer.writeheader()

            for result in results:

                writer.writerow({
                    key: result[key]
                    for key in fields
                })

    # =================================================
    # 7. FINAL SUMMARY
    # =================================================

    passed = sum(
        r["result"] == "PASS"
        for r in results
    )

    failed = len(results) - passed

    print("\n" + "=" * 55)
    print("FINAL VERIFICATION SUMMARY")
    print("=" * 55)

    print("Weather files found:", len(files))
    print("Files successfully checked:", len(results))
    print("Passed:", passed)
    print("Failed:", failed)

    if len(results) == EXPECTED_DATASETS and passed == EXPECTED_DATASETS:

        print("\nALL 9 WEATHER DATASETS PASSED.")

        print(
            "All 30 target coordinates are inside "
            "or on the selected DNCC + DSCC boundary."
        )

    else:

        print("\nVERIFICATION NOT FULLY PASSED.")

        print(
            "Review the individual dataset results "
            "and any skipped files."
        )

    print("\nReport saved at:")
    print(REPORT_FILE)


if __name__ == "__main__":
    main()
