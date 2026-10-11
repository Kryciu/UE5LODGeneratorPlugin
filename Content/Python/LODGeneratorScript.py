import unreal

def _collect_static_meshes_from_selection():
    eut = unreal.EditorUtilityLibrary
    eal = unreal.EditorAssetLibrary

    static_meshes = []

    selected_folders = []
    if hasattr(eut, "get_selected_folder_paths"):
        selected_folders = eut.get_selected_folder_paths() or []

    selected_assets = eut.get_selected_assets() or []

    for asset in selected_assets:
        if isinstance(asset, unreal.StaticMesh):
            static_meshes.append(asset)

    for folder in selected_folders:
        asset_paths = eal.list_assets(folder, recursive=True, include_folder=False)

        for path in asset_paths:
            asset = eal.load_asset(path)

            if isinstance(asset, unreal.StaticMesh):
                static_meshes.append(asset)

    if not static_meshes:
        asset_data_list = eut.get_selected_asset_data() or []

        for ad in asset_data_list:
            try:
                if ad.asset_class_path.asset_name == "StaticMesh" or ad.asset_class == "StaticMesh":
                    asset = eal.load_asset(ad.object_path_string)

                    if isinstance(asset, unreal.StaticMesh):
                        static_meshes.append(asset)
            except Exception:
                pass

    unique_meshes = []
    seen_paths = set()

    for mesh in static_meshes:
        path = mesh.get_path_name()

        if path not in seen_paths:
            unique_meshes.append(mesh)
            seen_paths.add(path)

    return unique_meshes


def level_of_detail_generator_from_selection(lod_group="None", lod_reduction_map=None):
    if lod_reduction_map is None:
        lod_reduction_map = {
            1.0: 1.0,
            0.5: 0.5,
            0.25: 0.25,
            0.125: 0.125,
        }

    static_meshes = _collect_static_meshes_from_selection()

    if not static_meshes:
        unreal.log_error(
            "No Static Meshes found from current selection. Select folders and/or Static Mesh assets and try again."
        )
        return

    total_frames = len(static_meshes)
    text_label = "Generating LOD..."

    with unreal.ScopedSlowTask(total_frames, text_label) as slow_task:
        slow_task.make_dialog(True)

        for static_mesh in static_meshes:
            if slow_task.should_cancel():
                unreal.log_warning("LOD Generation canceled by user")
                break

            slow_task.enter_progress_frame(1, f"Processing {static_mesh.get_name()}")

            reduction_options = unreal.EditorScriptingMeshReductionOptions()
            reduction_options.set_editor_property("auto_compute_lod_screen_size", False)

            if lod_group and lod_group != "None":
                static_mesh.set_editor_property("lod_group", unreal.Name(lod_group))

            reduction_settings = []

            if lod_reduction_map and len(lod_reduction_map) != 0:
                for percent_triangles, screen_size in sorted(
                    lod_reduction_map.items(),
                    key=lambda item: item[1],
                    reverse=True
                ):
                    lod_settings = unreal.EditorScriptingMeshReductionSettings()
                    lod_settings.set_editor_property("percent_triangles", float(percent_triangles))
                    lod_settings.set_editor_property("screen_size", float(screen_size))
                    reduction_settings.append(lod_settings)

                reduction_options.set_editor_property("reduction_settings", reduction_settings)
                unreal.EditorStaticMeshLibrary.set_lods(static_mesh, reduction_options)
            else:
                unreal.log_warning(
                    f"LOD Reduction Map is empty, only applying LOD Group for {static_mesh.get_name()}"
                )

            unreal.EditorAssetLibrary.save_asset(static_mesh.get_path_name())

    unreal.log("LOD Generation process complete.")