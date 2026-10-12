// Unit and statistical tests for the CPU-compiled kernel. Included by KernelTool.cpp only.
// Each test names the docs/VERIFY.md V-ID it provides evidence for.

namespace
{
    struct TEST_CONTEXT
    {
        int failures = 0;
        bool isquick = false;
    };

    struct TEST_CASE
    {
        const char* name;
        const char* verify_id;
        std::function<void(TEST_CONTEXT&)> body;
    };

    std::vector<TEST_CASE>& test_registry()
    {
        static std::vector<TEST_CASE> tests;
        return tests;
    }

    struct TEST_REGISTRAR
    {
        TEST_REGISTRAR(const char* name, const char* verify_id, std::function<void(TEST_CONTEXT&)> body)
        {
            test_registry().push_back({ name, verify_id, std::move(body) });
        }
    };

#define TEST(name, verify_id) \
    void name(TEST_CONTEXT& context); \
    TEST_REGISTRAR name##_registrar(#name, verify_id, name); \
    void name(TEST_CONTEXT& context)

#define CHECK(condition) \
    do { if (!(condition)) { ++context.failures; std::printf("    FAIL %s:%d  %s\n", __FILE__, __LINE__, #condition); } } while (0)

#define CHECK_NEAR(a, b, tolerance) \
    do { const double va = (a), vb = (b); if (!(std::fabs(va - vb) <= (tolerance))) { ++context.failures; \
        std::printf("    FAIL %s:%d  %s = %.7g, expected %.7g (tol %.3g)\n", __FILE__, __LINE__, #a, va, vb, double(tolerance)); } } while (0)

    bool near3(float3 a, float3 b, float tolerance)
    {
        return std::fabs(a.x - b.x) <= tolerance && std::fabs(a.y - b.y) <= tolerance && std::fabs(a.z - b.z) <= tolerance;
    }

    SCENE single_primitive_scene(PRIMITIVE primitive)
    {
        SCENE scene;
        MATERIAL material{};
        material.type = material_diffuse;
        material.albedo = { 0.5f, 0.5f, 0.5f };
        scene.materials.push_back(material);
        scene.material_names.push_back("gray");
        primitive.material = 0;
        scene.primitives.push_back(primitive);
        return scene;
    }

    // ------------------------------------------------------------------ V-NORMAL
    TEST(sphere_hit_from_outside_and_inside, "V-NORMAL")
    {
        const PRIMITIVE sphere = make_sphere({ 0, 0, 0 }, 1.0f, 0);
        float t; float3 n;
        CHECK(intersect_sphere(make_ray({ 0, 0, 5 }, { 0, 0, -1 }), sphere, 0.0f, float_max, t, n));
        CHECK_NEAR(t, 4.0, 1e-5);
        CHECK(near3(n, { 0, 0, 1 }, 1e-5f));
        CHECK(intersect_sphere(make_ray({ 0, 0, 0 }, { 0, 0, -1 }), sphere, 0.0f, float_max, t, n));
        CHECK_NEAR(t, 1.0, 1e-5);
        CHECK(near3(n, { 0, 0, -1 }, 1e-5f)); // still outward from inside
        CHECK(!intersect_sphere(make_ray({ 0, 2, 5 }, { 0, 0, -1 }), sphere, 0.0f, float_max, t, n));
        CHECK(!intersect_sphere(make_ray({ 0, 0, 5 }, { 0, 0, -1 }), sphere, 0.0f, 3.0f, t, n)); // beyond tmax
        // Far-away grazing precision: large offset keeps the stable quadratic accurate.
        const PRIMITIVE far_sphere = make_sphere({ 0, 0, -10000 }, 1.0f, 0);
        CHECK(intersect_sphere(make_ray({ 0, 0, 0 }, { 0, 0, -1 }), far_sphere, 0.0f, float_max, t, n));
        CHECK_NEAR(t, 9999.0, 1e-2);
    }

    TEST(quad_front_back_and_bounds, "V-NORMAL")
    {
        const PRIMITIVE floor = make_quad({ -1, 0, 1 }, { 2, 0, 0 }, { 0, 0, -2 }, 0);
        CHECK(near3(floor.normal, { 0, 1, 0 }, 1e-6f));
        CHECK_NEAR(floor.area, 4.0, 1e-6);
        float t; float3 n;
        CHECK(intersect_quad(make_ray({ 0.5f, 3, 0.5f }, { 0, -1, 0 }), floor, 0.0f, float_max, t, n));
        CHECK_NEAR(t, 3.0, 1e-5);
        CHECK(near3(n, { 0, 1, 0 }, 1e-6f));
        CHECK(intersect_quad(make_ray({ 0.5f, -3, 0.5f }, { 0, 1, 0 }), floor, 0.0f, float_max, t, n));
        CHECK(!intersect_quad(make_ray({ 1.5f, 3, 0 }, { 0, -1, 0 }), floor, 0.0f, float_max, t, n));
        CHECK(!intersect_quad(make_ray({ 0, 3, 0 }, { 1, 0, 0 }), floor, 0.0f, float_max, t, n)); // parallel
        // isfront_face through the scene query
        const SCENE scene = single_primitive_scene(floor);
        cpu_bind_scene(scene, 4, 4);
        HIT hit;
        CHECK(intersect_scene(make_ray({ 0, 3, 0 }, { 0, -1, 0 }), 0.0f, float_max, hit) && hit.isfront_face);
        CHECK(intersect_scene(make_ray({ 0, -3, 0 }, { 0, 1, 0 }), 0.0f, float_max, hit) && !hit.isfront_face);
    }

    TEST(rotated_box_faces_are_outward, "V-NORMAL")
    {
        const PRIMITIVE box = make_box({ -1, -2, -3 }, { 1, 2, 3 }, 30.0f, 0);
        CHECK_NEAR(box.area, 8.0 * (1 * 2 + 2 * 3 + 3 * 1), 1e-4);
        const float3 axes[3] = { box.edge_u, box.edge_v, box.normal };
        const float half[3] = { 1, 2, 3 };
        for (int axis = 0; axis < 3; ++axis)
            for (float side : { 1.0f, -1.0f })
            {
                const float3 outward = axes[axis] * side;
                float t; float3 n;
                const float3 origin = outward * (half[axis] + 5.0f);
                CHECK(intersect_box(make_ray(origin, -outward), box, 0.0f, float_max, t, n));
                CHECK_NEAR(t, 5.0, 1e-4);
                CHECK(near3(n, outward, 1e-4f));
                // From the center the exit face is the same face, normal still outward.
                CHECK(intersect_box(make_ray({ 0, 0, 0 }, outward), box, 0.0f, float_max, t, n));
                CHECK_NEAR(t, half[axis], 1e-4);
                CHECK(near3(n, outward, 1e-4f));
            }
        float t; float3 n;
        CHECK(!intersect_box(make_ray({ 10, 10, 10 }, { 1, 0, 0 }), box, 0.0f, float_max, t, n));
    }

    TEST(scene_query_returns_closest_hit, "V-NORMAL")
    {
        SCENE scene = single_primitive_scene(make_sphere({ 0, 0, -10 }, 1.0f, 0));
        scene.primitives.push_back(make_sphere({ 0, 0, -4 }, 1.0f, 0));
        scene.primitives.push_back(make_quad({ -5, -5, -20 }, { 10, 0, 0 }, { 0, 10, 0 }, 0));
        cpu_bind_scene(scene, 4, 4);
        HIT hit;
        CHECK(intersect_scene(make_ray({ 0, 0, 0 }, { 0, 0, -1 }), 0.0f, float_max, hit));
        CHECK(hit.primitive == 1u);
        CHECK_NEAR(hit.t, 3.0, 1e-5);
        CHECK(near3(hit.position, { 0, 0, -3 }, 1e-5f));
        CHECK(hit.isfront_face);
    }

    TEST(camera_rays_follow_the_convention, "V-NORMAL")
    {
        SCENE scene = single_primitive_scene(make_sphere({ 0, 0, 0 }, 1.0f, 0));
        scene.camera.position = { 0, 0, 5 };
        scene.camera.target = { 0, 0, 0 };
        scene.camera.fov_y = 90.0f;
        cpu_bind_scene(scene, 200, 100);
        // Right-handed, +Y up, looking down -Z: right must be +X.
        CHECK(near3(g_frame.camera_right, { 1, 0, 0 }, 1e-6f));
        CHECK(near3(g_frame.camera_up, { 0, 1, 0 }, 1e-6f));
        RAY center = generate_camera_ray(100.0f, 50.0f);
        CHECK(near3(center.direction, { 0, 0, -1 }, 1e-6f));
        RAY top = generate_camera_ray(100.0f, 0.0f); // top edge: 45 degrees up for fov 90
        CHECK_NEAR(top.direction.y, std::sqrt(0.5), 1e-5);
        RAY right = generate_camera_ray(200.0f, 50.0f); // right edge: tan = aspect * tan(45) = 2
        CHECK_NEAR(right.direction.x / -right.direction.z, 2.0, 1e-5);
    }

    // ------------------------------------------------------------------ V-SCENE
    TEST(cornell_box_loads, "V-SCENE")
    {
        SCENE scene;
        std::string error;
        CHECK(SCENE_LOADER::load_file(scene_path("cornell_box.json"), scene, error));
        if (!error.empty()) std::printf("    %s\n", error.c_str());
        CHECK(scene.primitives.size() == 8u);
        CHECK(scene.materials.size() == 4u);
        CHECK(scene.name == "Cornell Box");
        // Walls face into the box (front face toward the camera side).
        CHECK(near3(scene.primitives[0].normal, { 0, 1, 0 }, 1e-6f));
        CHECK(near3(scene.primitives[1].normal, { 0, -1, 0 }, 1e-6f));
        CHECK(near3(scene.primitives[2].normal, { 0, 0, 1 }, 1e-6f));
        CHECK(near3(scene.primitives[3].normal, { 1, 0, 0 }, 1e-6f));
        CHECK(near3(scene.primitives[4].normal, { -1, 0, 0 }, 1e-6f));
        CHECK(near3(scene.primitives[5].normal, { 0, -1, 0 }, 1e-6f));
    }

    TEST(invalid_scenes_are_rejected_and_keep_the_old_scene, "V-SCENE")
    {
        SCENE scene;
        std::string error;
        CHECK(SCENE_LOADER::load_file(scene_path("cornell_box.json"), scene, error));
        const char* base_camera = R"("camera": { "position": [0, 0, 5], "target": [0, 0, 0] })";
        const std::string material = R"("materials": { "m": { "type": "diffuse", "albedo": [0.5, 0.5, 0.5] }, "e": { "type": "emissive", "radiance": [1, 1, 1] } })";
        const std::vector<std::pair<std::string, std::string>> cases = {
            { "malformed json", "{ \"camera\": " },
            { "unknown top-level key", std::string("{ ") + base_camera + ", " + material + R"(, "primitives": [ { "type": "sphere", "material": "m", "center": [0,0,0], "radius": 1 } ], "extra": 1 })" },
            { "unknown material", std::string("{ ") + base_camera + ", " + material + R"(, "primitives": [ { "type": "sphere", "material": "nope", "center": [0,0,0], "radius": 1 } ] })" },
            { "negative radius", std::string("{ ") + base_camera + ", " + material + R"(, "primitives": [ { "type": "sphere", "material": "m", "center": [0,0,0], "radius": -1 } ] })" },
            { "emissive box", std::string("{ ") + base_camera + ", " + material + R"(, "primitives": [ { "type": "box", "material": "e", "min": [0,0,0], "max": [1,1,1] } ] })" },
            { "albedo above one", std::string("{ ") + base_camera + R"(, "materials": { "m": { "type": "diffuse", "albedo": [1.2, 0.5, 0.5] } }, "primitives": [ { "type": "sphere", "material": "m", "center": [0,0,0], "radius": 1 } ] })" },
            { "degenerate quad", std::string("{ ") + base_camera + ", " + material + R"(, "primitives": [ { "type": "quad", "material": "m", "corner": [0,0,0], "u": [1,0,0], "v": [2,0,0] } ] })" },
            { "missing camera", std::string("{ ") + material + R"(, "primitives": [ { "type": "sphere", "material": "m", "center": [0,0,0], "radius": 1 } ] })" },
            { "unknown primitive key", std::string("{ ") + base_camera + ", " + material + R"(, "primitives": [ { "type": "sphere", "material": "m", "center": [0,0,0], "radius": 1, "colour": 2 } ] })" },
        };
        for (const auto& [label, text] : cases)
        {
            std::string message;
            const bool isloaded = SCENE_LOADER::load_text(text, scene, message);
            if (isloaded) std::printf("    case '%s' unexpectedly loaded\n", label.c_str());
            CHECK(!isloaded);
            CHECK(!message.empty());
            CHECK(scene.primitives.size() == 8u); // previous scene untouched
        }
        std::string message;
        const std::string valid = std::string("{ ") + base_camera + ", " + material +
            R"(, "primitives": [ { "type": "sphere", "material": "m", "center": [0,0,0], "radius": 1 } ] })";
        CHECK(SCENE_LOADER::load_text(valid, scene, message));
        CHECK(scene.primitives.size() == 1u);
    }
    // ------------------------------------------------------------------ M2: sampling and accumulation
    TEST(random_numbers_are_in_unit_interval_and_uniform, "V-DETERMINISM")
    {
        RNG rng = make_rng(3, 7, 11, 0);
        double sum = 0.0;
        float lowest = 1.0f, highest = 0.0f;
        const int count = 200000;
        for (int index = 0; index < count; ++index)
        {
            const float u = next_float(rng);
            lowest = std::min(lowest, u);
            highest = std::max(highest, u);
            sum += u;
        }
        CHECK(lowest >= 0.0f);
        CHECK(highest < 1.0f);
        CHECK_NEAR(sum / count, 0.5, 0.005);
        RNG a = make_rng(1, 2, 3, 4), b = make_rng(1, 2, 3, 4), c = make_rng(1, 2, 3, 5);
        CHECK(next_float(a) == next_float(b));
        CHECK(next_float(a) != next_float(c));
    }

    TEST(cosine_sampling_matches_its_pdf, "V-NAN")
    {
        // E[cos] under p = cos/pi is 2/3; every direction lies in the upper hemisphere.
        RNG rng = make_rng(0, 0, 0, 1);
        double sum = 0.0;
        bool isupper = true;
        const int count = 200000;
        for (int index = 0; index < count; ++index)
        {
            const float3 d = sample_cosine_hemisphere(next_float(rng), next_float(rng));
            isupper = isupper && d.z >= 0.0f && std::fabs(length(d) - 1.0f) < 1e-4f;
            sum += d.z;
        }
        CHECK(isupper);
        CHECK_NEAR(sum / count, 2.0 / 3.0, 0.005);
        // Orthonormal frame for awkward normals (including -Z, the classic singularity).
        for (float3 n : { make_float3(0, 0, 1), make_float3(0, 0, -1), normalize(make_float3(1, 2, 3)), make_float3(0, -1, 0) })
        {
            const SHADING_FRAME frame = make_frame(n);
            CHECK_NEAR(dot(frame.tangent, frame.bitangent), 0.0, 1e-5);
            CHECK_NEAR(dot(frame.tangent, n), 0.0, 1e-5);
            CHECK(near3(cross(frame.tangent, frame.bitangent), n, 1e-5f));
            CHECK(near3(to_world(frame, to_local(frame, make_float3(0.3f, -0.2f, 0.9f))), make_float3(0.3f, -0.2f, 0.9f), 1e-5f));
        }
    }

    TEST(lambert_weight_equals_albedo, "V-NAN")
    {
        MATERIAL material{};
        material.type = material_diffuse;
        material.albedo = { 0.2f, 0.5f, 0.8f };
        const float3 wo = normalize(make_float3(0.3f, 0.1f, 0.9f));
        RNG rng = make_rng(0, 0, 0, 2);
        for (int index = 0; index < 1000; ++index)
        {
            BSDF_SAMPLE sample;
            CHECK(bsdf_sample(material, wo, next_float(rng), next_float(rng), next_float(rng), sample));
            const float3 f = bsdf_eval(material, wo, sample.direction);
            const float3 expected = f * sample.direction.z / sample.pdf;
            CHECK(near3(sample.weight, expected, 1e-4f));
            CHECK_NEAR(sample.pdf, bsdf_pdf(material, wo, sample.direction), 1e-5);
        }
        BSDF_SAMPLE sample;
        CHECK(!bsdf_sample(material, make_float3(0, 0, -1), 0.5f, 0.5f, 0.5f, sample)); // wo below the surface
    }

    TEST(path_length_respects_max_depth, "V-NAN")
    {
        SCENE scene;
        std::string error;
        CHECK(SCENE_LOADER::load_file(scene_path("cornell_box.json"), scene, error));
        cpu_bind_scene(scene, 32, 32);
        for (uint depth : { 1u, 2u, 5u })
        {
            g_frame.max_depth = depth;
            uint longest = 0;
            for (uint s = 0; s < 64; ++s)
            {
                uint length;
                render_sample(16, 16, s, length);
                longest = std::max(longest, length);
            }
            CHECK(longest <= depth);
        }
        // max_depth = 1: only emitters are visible, so the floor (center-bottom pixel) is black.
        g_frame.max_depth = 1;
        uint length;
        const float3 floor = render_sample(16, 30, 0, length);
        CHECK(floor.x == 0.0f && floor.y == 0.0f && floor.z == 0.0f);
    }

    TEST(cornell_box_render_is_deterministic, "V-DETERMINISM")
    {
        SCENE scene;
        std::string error;
        CHECK(SCENE_LOADER::load_file(scene_path("cornell_box.json"), scene, error));
        cpu_bind_scene(scene, 48, 48);
        const CPU_IMAGE first = cpu_render(8);
        const CPU_IMAGE second = cpu_render(8);
        CHECK(first.pixels.size() == second.pixels.size());
        CHECK(std::memcmp(first.pixels.data(), second.pixels.data(), first.pixels.size() * sizeof(float3)) == 0);
        g_frame.seed = 1;
        const CPU_IMAGE other_seed = cpu_render(8);
        CHECK(std::memcmp(first.pixels.data(), other_seed.pixels.data(), first.pixels.size() * sizeof(float3)) != 0);
    }

    TEST(cornell_box_has_no_non_finite_samples, "V-NAN")
    {
        SCENE scene;
        std::string error;
        CHECK(SCENE_LOADER::load_file(scene_path("cornell_box.json"), scene, error));
        const uint spp = context.isquick ? 64 : 1024;
        cpu_bind_scene(scene, 32, 32);
        const CPU_IMAGE image = cpu_render(spp);
        CHECK(image.invalid_samples == 0u);
        double sum = 0.0;
        for (const float3& p : image.pixels) sum += p.x + p.y + p.z;
        CHECK(std::isfinite(sum) && sum > 0.0);
        std::printf("    %u spp, 32x32: invalid samples %u, mean rgb sum %.4f\n", spp, image.invalid_samples,
            sum / image.pixels.size());
    }
}

int run_tests(const std::vector<std::string>& arguments)
{
    TEST_CONTEXT context;
    for (const auto& argument : arguments) if (argument == "--quick") context.isquick = true;
    int failed_tests = 0;
    for (const auto& test : test_registry())
    {
        const int before = context.failures;
        std::printf("[%-13s] %s\n", test.verify_id, test.name);
        test.body(context);
        if (context.failures != before) ++failed_tests;
    }
    std::printf("\n%zu tests, %d failed, %d failed checks\n", test_registry().size(), failed_tests, context.failures);
    return context.failures == 0 ? 0 : 1;
}
