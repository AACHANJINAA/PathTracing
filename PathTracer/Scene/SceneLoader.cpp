#include "SceneLoader.h"
#include "Json.h"

#include <cmath>
#include <fstream>
#include <initializer_list>
#include <iterator>
#include <sstream>

namespace
{
    // Throws std::string with a path-like context ("primitives[2].radius: ...").
    [[noreturn]] void fail(const std::string& where, const std::string& message)
    {
        throw where + ": " + message;
    }

    void check_keys(const JSON_VALUE& value, const std::string& where, std::initializer_list<const char*> allowed)
    {
        if (!value.is_object()) fail(where, "expected an object");
        for (const auto& entry : value.object)
        {
            bool isknown = false;
            for (const char* key : allowed) isknown = isknown || entry.first == key;
            if (!isknown) fail(where, "unknown key \"" + entry.first + "\"");
        }
    }

    const JSON_VALUE& require(const JSON_VALUE& object, const char* key, const std::string& where)
    {
        const JSON_VALUE* value = object.find(key);
        if (!value) fail(where, std::string("missing \"") + key + "\"");
        return *value;
    }

    float read_float(const JSON_VALUE& value, const std::string& where)
    {
        if (!value.is_number() || !std::isfinite(value.number)) fail(where, "expected a finite number");
        return static_cast<float>(value.number);
    }

    float3 read_float3(const JSON_VALUE& value, const std::string& where)
    {
        if (!value.is_array() || value.array.size() != 3) fail(where, "expected [x, y, z]");
        return { read_float(value.array[0], where + "[0]"), read_float(value.array[1], where + "[1]"),
            read_float(value.array[2], where + "[2]") };
    }

    std::string read_string(const JSON_VALUE& value, const std::string& where)
    {
        if (!value.is_string()) fail(where, "expected a string");
        return value.string;
    }

    float length3(float3 a) { return std::sqrt(a.x * a.x + a.y * a.y + a.z * a.z); }

    void read_camera(const JSON_VALUE& value, SCENE_CAMERA& camera)
    {
        const std::string where = "camera";
        check_keys(value, where, { "position", "target", "up", "fov_y" });
        camera.position = read_float3(require(value, "position", where), where + ".position");
        camera.target = read_float3(require(value, "target", where), where + ".target");
        if (const auto* up = value.find("up")) camera.up = read_float3(*up, where + ".up");
        if (const auto* fov = value.find("fov_y")) camera.fov_y = read_float(*fov, where + ".fov_y");
        if (camera.fov_y <= 1.0f || camera.fov_y >= 179.0f) fail(where + ".fov_y", "must be in (1, 179) degrees");
        const float3 forward{ camera.target.x - camera.position.x, camera.target.y - camera.position.y,
            camera.target.z - camera.position.z };
        if (length3(forward) < 1e-6f) fail(where, "position and target must differ");
        const float3 side{ forward.y * camera.up.z - forward.z * camera.up.y,
            forward.z * camera.up.x - forward.x * camera.up.z, forward.x * camera.up.y - forward.y * camera.up.x };
        if (length3(side) < 1e-6f) fail(where, "up must not be parallel to the view direction");
    }

    void read_color(const JSON_VALUE& value, const std::string& where, float limit, float3& color)
    {
        color = read_float3(value, where);
        for (float c : { color.x, color.y, color.z })
            if (c < 0.0f || c > limit) fail(where, "components must be in [0, " + std::to_string(limit) + "]");
    }

    void read_materials(const JSON_VALUE& value, SCENE& scene)
    {
        if (!value.is_object() || value.object.empty()) fail("materials", "expected a non-empty object of named materials");
        for (const auto& entry : value.object)
        {
            const std::string where = "materials." + entry.first;
            const JSON_VALUE& definition = entry.second;
            if (!definition.is_object()) fail(where, "expected an object");
            const std::string type = read_string(require(definition, "type", where), where + ".type");
            MATERIAL material{};
            material.ior = 1.0f;
            if (type == "diffuse")
            {
                check_keys(definition, where, { "type", "albedo" });
                material.type = material_diffuse;
                read_color(require(definition, "albedo", where), where + ".albedo", 1.0f, material.albedo);
            }
            else if (type == "emissive")
            {
                check_keys(definition, where, { "type", "radiance" });
                material.type = material_emissive;
                read_color(require(definition, "radiance", where), where + ".radiance", 1e6f, material.emission);
            }
            else fail(where + ".type", "unknown material type \"" + type + "\"");
            scene.material_names.push_back(entry.first);
            scene.materials.push_back(material);
        }
    }

    uint find_material(const SCENE& scene, const std::string& name, const std::string& where)
    {
        for (size_t index = 0; index < scene.material_names.size(); ++index)
            if (scene.material_names[index] == name) return static_cast<uint>(index);
        fail(where, "unknown material \"" + name + "\"");
    }

    void read_primitives(const JSON_VALUE& value, SCENE& scene)
    {
        if (!value.is_array() || value.array.empty()) fail("primitives", "expected a non-empty array");
        if (value.array.size() > SCENE_LOADER::max_primitives)
            fail("primitives", "more than " + std::to_string(SCENE_LOADER::max_primitives) + " primitives");
        for (size_t index = 0; index < value.array.size(); ++index)
        {
            const std::string where = "primitives[" + std::to_string(index) + "]";
            const JSON_VALUE& definition = value.array[index];
            if (!definition.is_object()) fail(where, "expected an object");
            const std::string type = read_string(require(definition, "type", where), where + ".type");
            const uint material = find_material(scene,
                read_string(require(definition, "material", where), where + ".material"), where + ".material");
            PRIMITIVE primitive{};
            if (type == "sphere")
            {
                check_keys(definition, where, { "type", "material", "center", "radius" });
                const float radius = read_float(require(definition, "radius", where), where + ".radius");
                if (radius <= 0.0f) fail(where + ".radius", "must be positive");
                primitive = make_sphere(read_float3(require(definition, "center", where), where + ".center"), radius, material);
            }
            else if (type == "quad")
            {
                check_keys(definition, where, { "type", "material", "corner", "u", "v" });
                primitive = make_quad(read_float3(require(definition, "corner", where), where + ".corner"),
                    read_float3(require(definition, "u", where), where + ".u"),
                    read_float3(require(definition, "v", where), where + ".v"), material);
                if (primitive.area < 1e-8f) fail(where, "u and v must span a non-zero area");
            }
            else if (type == "box")
            {
                check_keys(definition, where, { "type", "material", "min", "max", "rotation_y" });
                const float3 minimum = read_float3(require(definition, "min", where), where + ".min");
                const float3 maximum = read_float3(require(definition, "max", where), where + ".max");
                if (maximum.x <= minimum.x || maximum.y <= minimum.y || maximum.z <= minimum.z)
                    fail(where, "max must be greater than min on every axis");
                float rotation = 0.0f;
                if (const auto* r = definition.find("rotation_y")) rotation = read_float(*r, where + ".rotation_y");
                primitive = make_box(minimum, maximum, rotation, material);
                // Light sampling supports quads and spheres only (docs/RENDERER_SPEC.md 3.3).
                if (scene.materials[material].type == material_emissive)
                    fail(where, "emissive material is only allowed on sphere and quad");
            }
            else fail(where + ".type", "unknown primitive type \"" + type + "\"");
            scene.primitives.push_back(primitive);
        }
    }
}

bool SCENE_LOADER::load_text(const std::string& text, SCENE& scene, std::string& error)
{
    JSON_VALUE root;
    if (!parse_json(text, root, error)) return false;
    SCENE loaded;
    try
    {
        check_keys(root, "scene", { "name", "camera", "materials", "primitives" });
        if (const auto* name = root.find("name")) loaded.name = read_string(*name, "name");
        read_camera(require(root, "camera", "scene"), loaded.camera);
        read_materials(require(root, "materials", "scene"), loaded);
        read_primitives(require(root, "primitives", "scene"), loaded);
    }
    catch (const std::string& message)
    {
        error = message;
        return false;
    }
    scene = std::move(loaded);
    return true;
}

bool SCENE_LOADER::load_file(const std::filesystem::path& path, SCENE& scene, std::string& error)
{
    std::ifstream file(path, std::ios::binary);
    if (!file) { error = "Could not open " + path.string(); return false; }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    std::string text = buffer.str();
    if (text.size() > 4 * 1024 * 1024) { error = "Scene file is too large"; return false; }
    if (text.size() >= 3 && static_cast<unsigned char>(text[0]) == 0xEF &&
        static_cast<unsigned char>(text[1]) == 0xBB && static_cast<unsigned char>(text[2]) == 0xBF) text.erase(0, 3);
    SCENE loaded;
    if (!load_text(text, loaded, error)) { error = path.filename().string() + ": " + error; return false; }
    if (loaded.name.empty()) loaded.name = path.stem().string();
    scene = std::move(loaded);
    return true;
}
