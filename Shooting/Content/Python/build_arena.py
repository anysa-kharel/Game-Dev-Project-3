"""Create this project's native Unreal materials, sounds and editable map.

Runs automatically on editor startup, or through Tools > Execute Python Script.
Refreshes lighting and checkpoint markers in an existing map while keeping its
layout. Python is used only in the editor, never by the packaged game.
"""
from pathlib import Path
import re
import unreal

ASSET_ROOT = "/Game/ClockArena"
MATERIAL = ASSET_ROOT + "/Materials/M_ArenaDaylight"
MAP = "/Game/ClockArena/Maps/L_ClockArena"


def _project_dir():
    return Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))


def _material():
    if unreal.EditorAssetLibrary.does_asset_exist(MATERIAL):
        return
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    mat = tools.create_asset(
        "M_ArenaDaylight", ASSET_ROOT + "/Materials",
        unreal.Material, unreal.MaterialFactoryNew()
    )
    if mat is None:
        raise RuntimeError("Could not create M_ArenaDaylight.")
    edit = unreal.MaterialEditingLibrary
    tint = edit.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -500, -100)
    tint.set_editor_property("parameter_name", "Tint")
    tint.set_editor_property("default_value", unreal.LinearColor(0.1, 0.2, 0.3, 1.0))
    tint.set_editor_property("use_custom_primitive_data", True)
    tint.set_editor_property("primitive_data_index", 0)
    glow = edit.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -500, 120)
    glow.set_editor_property("parameter_name", "Glow")
    glow.set_editor_property("default_value", 0.0)
    glow.set_editor_property("use_custom_primitive_data", True)
    glow.set_editor_property("primitive_data_index", 4)
    ambient = edit.create_material_expression(mat, unreal.MaterialExpressionConstant, -700, 300)
    ambient.set_editor_property("r", 0.35)
    add = edit.create_material_expression(mat, unreal.MaterialExpressionAdd, -400, 220)
    multiply = edit.create_material_expression(mat, unreal.MaterialExpressionMultiply, -100, 120)
    roughness = edit.create_material_expression(mat, unreal.MaterialExpressionConstant, -200, 320)
    roughness.set_editor_property("r", 0.82)
    metallic = edit.create_material_expression(mat, unreal.MaterialExpressionConstant, -200, 430)
    metallic.set_editor_property("r", 0.05)
    links = [
        edit.connect_material_property(tint, "", unreal.MaterialProperty.MP_BASE_COLOR),
        edit.connect_material_expressions(tint, "", multiply, "A"),
        edit.connect_material_expressions(glow, "", add, "A"),
        edit.connect_material_expressions(ambient, "", add, "B"),
        edit.connect_material_expressions(add, "", multiply, "B"),
        edit.connect_material_property(multiply, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR),
        edit.connect_material_property(roughness, "", unreal.MaterialProperty.MP_ROUGHNESS),
        edit.connect_material_property(metallic, "", unreal.MaterialProperty.MP_METALLIC),
    ]
    if not all(links):
        raise RuntimeError("A material connection could not be created.")
    edit.recompile_material(mat)
    if not unreal.EditorAssetLibrary.save_asset(MATERIAL, only_if_is_dirty=False):
        raise RuntimeError("Could not save the shared material.")


def _sounds():
    source = _project_dir() / "Tools" / "AudioSource"
    tasks = []
    for wav in sorted(source.glob("*.wav")):
        destination = ASSET_ROOT + "/Audio/" + wav.stem
        if unreal.EditorAssetLibrary.does_asset_exist(destination):
            continue
        task = unreal.AssetImportTask()
        task.set_editor_property("filename", str(wav))
        task.set_editor_property("destination_path", ASSET_ROOT + "/Audio")
        task.set_editor_property("destination_name", wav.stem)
        task.set_editor_property("automated", True)
        task.set_editor_property("replace_existing", False)
        task.set_editor_property("save", True)
        tasks.append(task)
    if tasks:
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
        for task in tasks:
            if not task.get_editor_property("imported_object_paths"):
                unreal.log_warning("An optional arena sound did not import.")


def _set_default_maps():
    ini = _project_dir() / "Config" / "DefaultEngine.ini"
    original = ini.read_text(encoding="utf-8")
    value = MAP + ".L_ClockArena"
    updated = re.sub(r"(?m)^EditorStartupMap=.*$", "EditorStartupMap=" + value, original)
    updated = re.sub(r"(?m)^GameDefaultMap=.*$", "GameDefaultMap=" + value, updated)
    if updated != original:
        ini.write_text(updated, encoding="utf-8")


def _map():
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    if unreal.EditorAssetLibrary.does_asset_exist(MAP):
        world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
        if world is None or world.get_name() != "L_ClockArena":
            if not levels.load_level(MAP):
                raise RuntimeError("Could not open the saved Clock Arena map.")
        refreshed = 0
        for actor in actors.get_all_level_actors():
            if actor.get_class().get_path_name() == "/Script/ShootingExpansion.CAArenaBuilder":
                actor.refresh_presentation()
                refreshed += 1
        if not refreshed:
            raise RuntimeError("The saved arena has no CAArenaBuilder to refresh. Your existing map was preserved.")
        if not levels.save_current_level():
            raise RuntimeError("Could not save the lighting and checkpoint upgrade.")
        _set_default_maps()
        return False
    builder_class = unreal.load_class(None, "/Script/ShootingExpansion.CAArenaBuilder")
    if builder_class is None:
        raise RuntimeError("The ShootingExpansion C++ module is not compiled or loaded.")
    if not levels.new_level(MAP):
        raise RuntimeError("Could not create the Clock Arena map.")
    builder = actors.spawn_actor_from_class(builder_class, unreal.Vector(0, 0, 0))
    if builder is None:
        raise RuntimeError("Could not place the native level builder.")
    builder.set_actor_label("CLOCK ARENA - Select to rebuild the expansion level")
    builder.build_arena()
    if not levels.save_current_level():
        raise RuntimeError("Could not save the generated map.")
    try:
        editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
        editor.set_level_viewport_camera_info(
            unreal.Vector(4000, -7200, 4800),
            unreal.Rotator(pitch=-28, yaw=85, roll=0)
        )
    except Exception:
        pass  # Viewport positioning is cosmetic.
    _set_default_maps()
    return True


def prepare():
    for path in ["/Game/InfimaGames/FreeFPSTemplate/Core/BP_Character", "/Game/InfimaGames/FreeFPSTemplate/Core/BP_Bullet"]:
        if not unreal.EditorAssetLibrary.does_asset_exist(path):
            raise RuntimeError("The template is missing a required asset: " + path)
    _material()
    _sounds()
    created = _map()
    unreal.log(
        "CLOCK ARENA READY / DAYLIGHT + CHECKPOINT FLAGS. Press Play, then click the viewport to start."
        + (" Native map, material and sounds have been generated." if created else "")
    )


if __name__ == "__main__":
    prepare()
