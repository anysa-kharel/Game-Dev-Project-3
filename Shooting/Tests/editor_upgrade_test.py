"""Check that editor preparation refreshes saved layouts instead of rebuilding them.

These tests use a small stand-in for editor subsystems; they do not run Unreal.
"""
import importlib.util
from pathlib import Path
import sys
from types import SimpleNamespace
import unittest
from unittest.mock import Mock, patch


class SavedMapUpgrade(unittest.TestCase):
    def setUp(self):
        self.builder = SimpleNamespace(
            get_class=lambda: SimpleNamespace(get_path_name=lambda: "/Script/ShootingExpansion.CAArenaBuilder"),
            refresh_presentation=Mock(), build_arena=Mock(), set_actor_label=Mock(),
        )
        self.levels = SimpleNamespace(load_level=Mock(return_value=True), new_level=Mock(return_value=True),
                                      save_current_level=Mock(return_value=True))
        self.actors = SimpleNamespace(get_all_level_actors=Mock(return_value=[self.builder]),
                                      spawn_actor_from_class=Mock(return_value=self.builder))
        self.editor = SimpleNamespace(get_editor_world=lambda: SimpleNamespace(get_name=lambda: "L_ClockArena"),
                                      set_level_viewport_camera_info=Mock())
        self.unreal = SimpleNamespace(
            LevelEditorSubsystem="levels", EditorActorSubsystem="actors", UnrealEditorSubsystem="editor",
            EditorAssetLibrary=SimpleNamespace(does_asset_exist=Mock(return_value=True)),
            load_class=Mock(return_value="builder class"), Vector=lambda *v: v, Rotator=lambda **v: v,
        )
        self.unreal.get_editor_subsystem=lambda key: {"levels":self.levels,"actors":self.actors,"editor":self.editor}[key]
        self.modules = patch.dict(sys.modules,{"unreal":self.unreal})
        self.modules.start()
        source=Path(__file__).resolve().parents[1]/"Content/Python/build_arena.py"
        spec=importlib.util.spec_from_file_location("upgrade_under_test",source)
        self.module=importlib.util.module_from_spec(spec)
        spec.loader.exec_module(self.module)
        self.defaults=patch.object(self.module,"_set_default_maps")
        self.defaults.start()

    def tearDown(self):
        self.defaults.stop()
        self.modules.stop()

    def test_saved_map_gets_refreshed_and_saved_without_rebuild(self):
        self.assertFalse(self.module._map())
        self.builder.refresh_presentation.assert_called_once()
        self.builder.build_arena.assert_not_called()
        self.levels.new_level.assert_not_called()
        self.levels.save_current_level.assert_called_once()

    def test_second_upgrade_still_preserves_layout(self):
        self.module._map()
        self.module._map()
        self.assertEqual(self.builder.refresh_presentation.call_count,2)
        self.builder.build_arena.assert_not_called()
        self.actors.spawn_actor_from_class.assert_not_called()

    def test_another_open_level_is_replaced_by_the_saved_arena(self):
        self.editor.get_editor_world=lambda: SimpleNamespace(get_name=lambda: "Entry")
        self.module._map()
        self.levels.load_level.assert_called_once_with(self.module.MAP)
        self.builder.refresh_presentation.assert_called_once()

    def test_failed_load_does_not_modify_the_wrong_map(self):
        self.editor.get_editor_world=lambda: None
        self.levels.load_level.return_value=False
        with self.assertRaises(RuntimeError): self.module._map()
        self.builder.refresh_presentation.assert_not_called()
        self.levels.save_current_level.assert_not_called()

    def test_missing_builder_preserves_the_saved_map(self):
        self.actors.get_all_level_actors.return_value=[]
        with self.assertRaises(RuntimeError): self.module._map()
        self.levels.save_current_level.assert_not_called()
        self.levels.new_level.assert_not_called()

    def test_failed_save_reports_failure(self):
        self.levels.save_current_level.return_value=False
        with self.assertRaises(RuntimeError): self.module._map()

    def test_new_project_builds_a_map_once(self):
        self.unreal.EditorAssetLibrary.does_asset_exist.return_value=False
        self.assertTrue(self.module._map())
        self.levels.new_level.assert_called_once_with(self.module.MAP)
        self.builder.build_arena.assert_called_once()
        self.levels.save_current_level.assert_called_once()

    def test_failed_new_map_does_not_spawn_geometry(self):
        self.unreal.EditorAssetLibrary.does_asset_exist.return_value=False
        self.levels.new_level.return_value=False
        with self.assertRaises(RuntimeError): self.module._map()
        self.actors.spawn_actor_from_class.assert_not_called()


if __name__=="__main__": unittest.main(verbosity=1)
