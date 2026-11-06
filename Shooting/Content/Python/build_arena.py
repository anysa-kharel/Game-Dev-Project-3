"""Create this project's native Unreal materials, sounds and editable map.

Runs automatically on editor startup, or through Tools > Execute Python Script.
Creates the longer campaign in a separate map. Existing campaign layouts are
refreshed; the original L_ClockArena map is retained. Python runs only in the editor.
"""
from pathlib import Path
import re
import unreal

ASSET_ROOT = "/Game/ClockArena"
MATERIAL = ASSET_ROOT + "/Materials/M_ArenaDaylight"
MAP = "/Game/ClockArena/Maps/L_ClockCampaign"
SIGN_MATERIAL = ASSET_ROOT + "/Materials/M_SignSurface"
PRESENTATION_AUDIO = {"S_MenuLoop": True, "S_GameAmbience": True, "S_GateOpen": False, "S_Reload": False}


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


def _sign_material():
    if unreal.EditorAssetLibrary.does_asset_exist(SIGN_MATERIAL):
        return
    mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "M_SignSurface", ASSET_ROOT + "/Materials", unreal.Material, unreal.MaterialFactoryNew()
    )
    if mat is None:
        raise RuntimeError("Could not create the readable-sign material.")
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property("two_sided", True)
    edit = unreal.MaterialEditingLibrary
    tint = edit.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -250, 0)
    tint.set_editor_property("parameter_name", "PanelTint")
    tint.set_editor_property("default_value", unreal.LinearColor(0.006, 0.010, 0.018, 1.0))
    tint.set_editor_property("use_custom_primitive_data", True)
    tint.set_editor_property("primitive_data_index", 0)
    if not edit.connect_material_property(tint, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR):
        raise RuntimeError("Could not connect the readable-sign material.")
    edit.recompile_material(mat)
    if not unreal.EditorAssetLibrary.save_asset(SIGN_MATERIAL, only_if_is_dirty=False):
        raise RuntimeError("Could not save the readable-sign material.")


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
    # These sounds are part of this update. Persist loop/virtualization settings
    # before Play so silent crossfade tracks can resume and packaged audio loops.
    for name, looping in PRESENTATION_AUDIO.items():
        path = ASSET_ROOT + "/Audio/" + name
        sound = unreal.EditorAssetLibrary.load_asset(path)
        if sound is None:
            raise RuntimeError("Missing required audio: " + name + ". Copy Tools/AudioSource from this update and rerun preparation.")
        changed = sound.get_editor_property("looping") != looping
        if changed:
            sound.set_editor_property("looping", looping)
        if looping:
            mode = unreal.VirtualizationMode.PLAY_WHEN_SILENT
            if sound.get_editor_property("virtualization_mode") != mode:
                sound.set_editor_property("virtualization_mode", mode)
                changed = True
        if changed and not unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False):
            raise RuntimeError("Could not save audio settings: " + name)


def _set_default_maps():
    ini = _project_dir() / "Config" / "DefaultEngine.ini"
    original = ini.read_text(encoding="utf-8")
    value = MAP + "." + MAP.rsplit("/", 1)[-1]
    updated = re.sub(r"(?m)^EditorStartupMap=.*$", "EditorStartupMap=" + value, original)
    updated = re.sub(r"(?m)^GameDefaultMap=.*$", "GameDefaultMap=" + value, updated)
    if updated != original:
        ini.write_text(updated, encoding="utf-8")


def _map():
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    if unreal.EditorAssetLibrary.does_asset_exist(MAP):
        world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
        if world is None or world.get_name() != "L_ClockCampaign":
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
            raise RuntimeError("Could not save the readable-sign upgrade.")
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
    builder.set_actor_label("CITADEL RUN - Five-sector campaign builder")
    builder.build_campaign()
    if not levels.save_current_level():
        raise RuntimeError("Could not save the generated map.")
    try:
        editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
        editor.set_level_viewport_camera_info(
            unreal.Vector(-4200, -6200, 4200),
            unreal.Rotator(pitch=-25, yaw=55, roll=0)
        )
    except Exception:
        pass  # Viewport positioning is cosmetic.
    _set_default_maps()
    return True


def prepare():
    if unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world() is not None:
        raise RuntimeError("Stop Play or Simulate before preparing the campaign map.")
    for path in ["/Game/InfimaGames/FreeFPSTemplate/Core/BP_Character", "/Game/InfimaGames/FreeFPSTemplate/Core/BP_Bullet"]:
        if not unreal.EditorAssetLibrary.does_asset_exist(path):
            raise RuntimeError("The template is missing a required asset: " + path)
    _material()
    _sign_material()
    _sounds()
    created = _map()
    unreal.log(
        "TICK TOCK READY / AMMO LABEL + RELOAD AUDIO / 5 SECTORS / 4 CLOCK BRIDGES / 60 ROBOTS. Press Play, then START or ENTER. M toggles music."
        + (" Native map, material and sounds have been generated." if created else "")
    )


if __name__ == "__main__":
    prepare()
