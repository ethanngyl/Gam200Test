#pragma once
/**
===============================================================================
 File:          MapSerializer.h
 Author:        Josh Ong
 Date:          2026
 ------------------------------------------------------------------------------

 MAP SERIALIZER - Save & Load Generated Maps

 Brief:
    Serializes GeneratedMap data to/from JSON files using nlohmann/json.
    Allows saving procedurally generated maps that you like, then loading
    them later as static levels -- bypassing the procedural generator.

 Saved Data:
    - Map dimensions and algorithm used
    - Full tile grid (WALL/FLOOR)
    - Player spawn position
    - Goal spawn position
    - All enemy spawn positions
    - All chest spawn positions
    - Boss arena data (if present)
    - Generation config (for reference/re-tweaking)

 File Format:
    JSON with .map.json extension. Human-readable and editable.

 Usage:
    // Save after generation
    MapGen::GeneratedMap map = generator.generate(config);
    MapSerializer::Save(map, config, "assets/maps/cool_dungeon.map.json");

    // Load later
    MapGen::GeneratedMap loaded;
    MapGen::Config loadedConfig;
    if (MapSerializer::Load("assets/maps/cool_dungeon.map.json", loaded, loadedConfig)) {
        // Use 'loaded' exactly like a freshly generated map
        ProceduralMapLoader::LoadFromGeneratedMap(loaded, spawner, em, ...);
    }

    // Quick save with auto-naming (timestamp-based)
    std::string filename = MapSerializer::QuickSave(map, config, "assets/maps/");


 Copyright (C) 2026 DigiPen Institute of Technology.
 Reproduction or disclosure of this file or its contents
 without the prior written consent of DigiPen Institute of
 Technology is prohibited.
===============================================================================
*/

#pragma once
#include "MapGenerator/MapGenerator.h"
#include <string>

namespace Framework {

    class MapSerializer {
    public:
        /**
         * @brief Save a generated map to a JSON file
         * @param map       The generated map to save
         * @param config    The config used to generate it (saved for reference)
         * @param filepath  Output file path (e.g. "assets/maps/my_map.map.json")
         * @return true if saved successfully
         */
        static bool Save(
            const MapGen::GeneratedMap& map,
            const MapGen::Config& config,
            const std::string& filepath
        );

        /**
         * @brief Load a generated map from a JSON file
         * @param filepath      Input file path
         * @param outMap        [out] The loaded map
         * @param outConfig     [out] The config that was used to generate it
         * @return true if loaded successfully
         */
        static bool Load(
            const std::string& filepath,
            MapGen::GeneratedMap& outMap,
            MapGen::Config& outConfig
        );

        /**
         * @brief Quick save with auto-generated filename (timestamp-based)
         * @param map       The generated map to save
         * @param config    The config used to generate it
         * @param directory Directory to save in (e.g. "assets/maps/")
         * @return The full filepath of the saved file, or "" on failure
         */
        static std::string QuickSave(
            const MapGen::GeneratedMap& map,
            const MapGen::Config& config,
            const std::string& directory = "assets/maps/"
        );

        /**
         * @brief Check if a saved map file exists and is valid
         * @param filepath  File path to check
         * @return true if the file exists and contains valid map data
         */
        static bool IsValidMapFile(const std::string& filepath);

        /**
         * @brief List all .map.json files in a directory
         * @param directory  Directory to scan
         * @return Vector of file paths
         */
        static std::vector<std::string> ListSavedMaps(const std::string& directory = "assets/maps/");

    private:
        static const int SERIALIZER_VERSION = 1;
    };

} // namespace Framework