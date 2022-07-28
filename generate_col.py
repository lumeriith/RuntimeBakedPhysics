import unreal

loaded = unreal.EditorUtilityLibrary.get_selected_assets()
static_meshes = unreal.EditorFilterLibrary.by_class(loaded, unreal.StaticMesh)

for sm in static_meshes:
    unreal.EditorStaticMeshLibrary.set_convex_decomposition_collisions(
        sm, 4, 16, 100000)
    unreal.EditorAssetLibrary.save_loaded_asset(sm)
    continue
    shape_type = unreal.ScriptingCollisionShapeType.NDOP18
    unreal.EditorStaticMeshLibrary.add_simple_collisions(sm, shape_type)
    unreal.EditorAssetLibrary.save_loaded_asset(sm)
